/*************************************************************************************
 *
 * @ Filename	 : FKGrpcConnectionPool.h
 * @ Description : FKGrpcServiceConfig:  gRPC配置定义
 *                 FKGrpcConnectionPool: gRPC连接池实现，管理不同的gRPC服务连接的创建、获取和回收
 *
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/21
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By:
 * Modifications:
 * ======================================
*************************************************************************************/
#ifndef FK_GRPC_CONNECTION_POOL_H_
#define FK_GRPC_CONNECTION_POOL_H_

#include <grpcpp/grpcpp.h>
#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <print>

struct FKGrpcServiceConfig {
	std::string host;                // 服务器地址
	int port;                        // 服务器端口
	size_t poolSize;                 // 连接池大小
	bool useSSL = false;             // 是否使用SSL
	std::chrono::seconds timeout{ 10 }; // 连接超时时间
    std::string getAddress() const {
        return std::format("{}:{}", host, port);
    }
};

template <typename ServiceType>
class FKGrpcConnectionPool {
public:
    using StubType = typename ServiceType::Stub;
    FKGrpcConnectionPool() : _pInitialized(false), _pActiveConnections(0) {}
    ~FKGrpcConnectionPool() { shutdown(); }

    void initialize(FKGrpcServiceConfig&& config) {
        std::lock_guard<std::mutex> lock(_pMutex);

        if (_pInitialized) {
            std::println("连接池已初始化，请勿重复初始化");
            return;
        }

        _pConfig = std::move(config);
        _pInitialized = true;
        // 预创建连接
        try {
            for (size_t i = 0; i < _pConfig.poolSize; ++i) {
                auto connection = _createStub();
                if (connection) {
                    _pConnections.push(std::move(connection));
                }
            }

            std::println("gRPC连接池初始化成功，服务地址: {}，连接池大小: {}", 
                _pConfig.getAddress(), _pConfig.poolSize);
        }
        catch (const std::exception& e) {
            _pInitialized = false;
            std::println("gRPC连接池初始化失败: {}", e.what());
            throw;
        }
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(_pMutex);

        if (!_pInitialized) return;

        // 标记为未初始化状态，阻止新的连接获取
        _pInitialized = false;

        // 清空连接队列
        while (!_pConnections.empty()) {
            _pConnections.pop();
        }

        _pActiveConnections.store(0);
        _pCv.notify_all();

        std::println("gRPC连接池已关闭");
    }

    size_t getActiveConnectionCount() const { return _pActiveConnections.load(); }
    
    size_t getAvailableConnectionCount() {
        std::lock_guard<std::mutex> lock(_pMutex);
        return _pConnections.size();
    }
    
    size_t getPoolSize() const { return _pConfig.poolSize; }

	std::string getStatus() const {
		std::lock_guard<std::mutex> lock(_pMutex);
		return std::format("Active: {}\nAvailable: {}\nPoolSize: {}",
			_pActiveConnections.load(),
			_pConnections.size(),
			_pConfig.poolSize);
	}

    std::unique_ptr<StubType> getConnection(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_lock<std::mutex> lock(_pMutex);

        if (!_pInitialized) {
            throw std::runtime_error("连接池未初始化，请先调用initialize方法");
        }

        // 等待可用连接或超时
        bool hasConnection = _pCv.wait_for(lock, timeout, [this] {
            return !_pConnections.empty() || !_pInitialized;
        });

        // 再次检查初始化状态（可能在等待期间被关闭）
        if (!_pInitialized) {
            throw std::runtime_error("连接池已关闭");
        }

        // 连接池为空，需要创建新连接
        if (!hasConnection) {
            std::println("获取连接超时，创建新连接 (活跃连接数: {})", _pActiveConnections.load());
            // 检查是否超过最大连接数限制
            if (_pActiveConnections.load() >= _pConfig.poolSize * 3) {
                throw std::runtime_error("已达到最大连接数限制，无法创建新连接");
            }

            // 创建新连接前先释放锁，避免长时间持有锁
            lock.unlock();
            // 创建新连接
            auto newConnection = _createStub();

            ++_pActiveConnections;
            return newConnection;
        }

        // 获取一个连接
        auto connection = std::move(_pConnections.front());
        _pConnections.pop();

        ++_pActiveConnections;
        return connection;
    }

    void releaseConnection(std::unique_ptr<StubType> connection) {
        if (!connection) return;

        std::lock_guard<std::mutex> lock(_pMutex);

        if (!_pInitialized)  return; 

        // 将连接放回队列
        _pConnections.push(std::move(connection));
        --_pActiveConnections;
        _pCv.notify_one();
    }

    template<typename Func>
    auto executeWithConnection(Func operation, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_ptr<StubType> connection;
        try {
            connection = getConnection(timeout);
        }
        catch (const std::exception& e) {
            std::println("获取gRPC连接失败: {}", e.what());
            throw;
        }
        
        if (!connection) {
            throw std::runtime_error("获取到无效的gRPC连接");
        }
        
        // 执行操作
        try {
            auto startTime = std::chrono::steady_clock::now();
            // 执行操作
            auto result = operation(connection.get());
            auto endTime = std::chrono::steady_clock::now();
            // 计算执行时间
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            if (duration > std::chrono::milliseconds(1000)) {
                std::println("gRPC调用耗时较长: {}ms", duration.count());
            }
            
            releaseConnection(std::move(connection));
            return result;
        }
        catch (const std::exception& e) {
            std::println("gRPC操作执行异常: {}", e.what());
            releaseConnection(std::move(connection));
            throw;
        }
        catch (...) {
            std::println("gRPC操作执行未知异常");
            releaseConnection(std::move(connection));
            throw;
        }
    }

private:
    std::unique_ptr<StubType> _createStub() {
        try {
            std::shared_ptr<grpc::Channel> channel;
            grpc::ChannelArguments channelArgs;

			// 设置通道参数
			channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);  // 30秒发送keepalive ping
			channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);  // 10秒超时
			channelArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);  // 允许在没有调用时发送keepalive
			channelArgs.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);  // 允许无限制的ping
			channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 10000);  // 最大重连间隔10秒

            // 根据配置创建安全或非安全通道
            if (_pConfig.useSSL) {
                grpc::SslCredentialsOptions sslOpts;
				// 可以在这里设置SSL证书路径
		        // ssl_opts.pem_root_certs = "..."
		        // ssl_opts.pem_private_key = "..."
		        // ssl_opts.pem_cert_chain = "..."
                auto creds = grpc::SslCredentials(sslOpts);
                channel = grpc::CreateCustomChannel(_pConfig.getAddress(), creds, channelArgs);
            }
            else {
                channel = grpc::CreateCustomChannel(_pConfig.getAddress(), grpc::InsecureChannelCredentials(), channelArgs);
            }

            auto stub = ServiceType::NewStub(channel);
            return stub;
        }
        catch (const std::exception& e) {
            std::println("创建gRPC连接失败: {}", e.what());
            throw;
        }
    }

    std::queue<std::unique_ptr<StubType>> _pConnections;
    mutable std::mutex _pMutex;
    std::condition_variable _pCv;
    FKGrpcServiceConfig _pConfig;
    std::atomic<bool> _pInitialized;
    std::atomic<size_t> _pActiveConnections;
};

#endif // !FK_GRPC_CONNECTION_POOL_H_