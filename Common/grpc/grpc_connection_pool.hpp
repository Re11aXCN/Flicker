/*************************************************************************************
 *
 * @ Filename     : GrpcConnectionPool.h
 * @ Description : GrpcServiceConfig:  grpc配置定义
 *                 GrpcConnectionPool: grpc连接池实现，管理不同的grpc服务连接的创建、获取和回收
 *
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/21
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By:
 * Modifications:
 * ======================================
*************************************************************************************/
#ifndef GRPC_CONNECTION_POOL_H_
#define GRPC_CONNECTION_POOL_H_

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <chrono>

#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4267)
#include <grpcpp/grpcpp.h>
#pragma warning(pop)

#include "Common/logger/logger_defend.h"
#include "Common/config/struct_config.h"

struct GrpcServiceConfig;
template <typename ServiceType>
class GrpcConnectionPool {
public:
    using StubType = typename ServiceType::Stub;
    GrpcConnectionPool(const GrpcServiceConfig& config) 
        : _pConfig(config)
    {
        // 预创建连接
        try {
            for (size_t i = 0; i < _pConfig.PoolSize; ++i) {
                auto connection = _createStub();
                if (connection) {
                    _pConnections.push(std::move(connection));
                }
            }

            LOGGER_INFO(std::format("grpc连接池初始化成功，服务地址: {}，连接池大小: {}", _pConfig.getAddress(), _pConfig.PoolSize));
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("grpc连接池初始化失败: {}", e.what()));
            throw;
        }
    }
    ~GrpcConnectionPool() { shutdown(); }

    void shutdown() {
        std::lock_guard<std::mutex> lock(_pMutex);
        if (_pShutdown) return;

        // 清空连接队列
        while (!_pConnections.empty()) {
            _pConnections.pop();
        }

        _pActiveConnections.store(0);
        _pCv.notify_all();

        LOGGER_INFO("grpc连接池已关闭!!!");
    }

    size_t getActiveConnectionCount() const { return _pActiveConnections.load(); }
    
    size_t getAvailableConnectionCount() {
        std::lock_guard<std::mutex> lock(_pMutex);
        return _pConnections.size();
    }
    
    size_t getPoolSize() const { return _pConfig.PoolSize; }

    std::string getStatus() const {
        std::lock_guard<std::mutex> lock(_pMutex);
        return std::format("Active: {}\nAvailable: {}\nPoolSize: {}",
            _pActiveConnections.load(),
            _pConnections.size(),
            _pConfig.PoolSize);
    }

    std::unique_ptr<StubType> getConnection() {
        std::unique_lock<std::mutex> lock(_pMutex);

        // 添加关闭状态检查
        if (_pShutdown) {
            throw std::runtime_error("连接池已关闭");
        }

        // 等待可用连接或超时
        bool hasConnection = _pCv.wait_for(lock, _pConfig.GrpclbCallTimeout, [this] {
            return _pShutdown || !_pConnections.empty();
            });

        // 再次检查关闭状态（可能在等待期间关闭）
        if (_pShutdown) {
            throw std::runtime_error("连接池已关闭");
        }

        // 连接池为空，需要创建新连接
        if (!hasConnection) {
            LOGGER_WARN(std::format("获取连接超时，创建新连接 (活跃连接数: {})", _pActiveConnections.load()));
            // 检查是否超过最大连接数限制
            if (_pActiveConnections.load() > _pConfig.PoolSize) {
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
        // 如果已关闭，直接丢弃连接
        if (_pShutdown) return; 

        // 将连接放回队列
        _pConnections.push(std::move(connection));
        --_pActiveConnections;
        _pCv.notify_one();
    }

    template<typename Func>
    auto executeWithConnection(Func operation) -> decltype(operation(std::declval<StubType*>())) {
        std::unique_ptr<StubType> connection;
        try {
            connection = getConnection();
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("获取grpc连接失败: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        }
        
        if (!connection) {
            throw std::runtime_error("获取到无效的grpc连接");
        }
        
        // 执行操作
        try {
            auto startTime = std::chrono::steady_clock::now();
            // 执行操作
            auto result = operation(connection.get());
            auto endTime = std::chrono::steady_clock::now();
            // 计算执行时间
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            if (duration.count() > 100) { // 只记录耗时超过100ms的调用
                LOGGER_INFO(std::format("grpc调用耗时较长: {}ms", duration.count()));
            }
            
            releaseConnection(std::move(connection));
            return result;
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("grpc操作执行异常: {}", e.what()));
            releaseConnection(std::move(connection));
            throw; // 重新抛出异常，让调用者处理
        }
        catch (...) {
            LOGGER_ERROR("grpc操作执行未知异常");
            releaseConnection(std::move(connection));
            throw; // 重新抛出未知异常，让调用者处理
        }
    }

private:
    std::unique_ptr<StubType> _createStub() {
        try {
            std::shared_ptr<grpc::Channel> channel;
            grpc::ChannelArguments channelArgs;

            // 设置通道参数
            //< 发送keepalive ping
            //< 超时
            //< 允许在没有调用时发送keepalive
            //< 允许无限制的ping
            //< 最大重连间隔
            channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, static_cast<int>(_pConfig.KeepAliveTime.count()));                       
            channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, static_cast<int>(_pConfig.KeepAliveTimeout.count()));
            channelArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, _pConfig.KeepAlivePermitWithoutCalls);
            channelArgs.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, _pConfig.Http2MaxPingWithoutData);
            channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, static_cast<int>(_pConfig.MaxReconnectBackoff.count()));
            channelArgs.SetInt(GRPC_ARG_GRPCLB_CALL_TIMEOUT_MS, static_cast<int>(_pConfig.GrpclbCallTimeout.count()));
            
            // 根据配置创建安全或非安全通道
            if (_pConfig.UseSSL) {
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
            LOGGER_ERROR(std::format("创建grpc连接失败: {}", e.what()));
            throw;
        }
    }

    std::queue<std::unique_ptr<StubType>> _pConnections;
    mutable std::mutex _pMutex;
    std::condition_variable _pCv;
    GrpcServiceConfig _pConfig;
    std::atomic<bool> _pShutdown{ false };
    std::atomic<size_t> _pActiveConnections{ 0 };
};

#endif // !GRPC_CONNECTION_POOL_H_