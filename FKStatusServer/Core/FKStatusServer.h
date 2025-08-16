#ifndef FK_STATUS_SERVER_H_
#define FK_STATUS_SERVER_H_

#include <memory>
#include <atomic>
#include <vector>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>

#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4267)
#include "Flicker/Global/Grpc/FKGrpcService.grpc.pb.h"
#pragma warning(pop)
#include "FKConfig.h"

class FKStatusServer : public std::enable_shared_from_this<FKStatusServer> {
public:
    explicit FKStatusServer(boost::asio::io_context& ioc, std::string && endpoint);
    ~FKStatusServer();

    void start();
    void stop();
    bool isRunning() const { return _pIsRunning.load(); }

private:
    void _incrementActiveConnections();
    void _decrementActiveConnections();
    void _grpcServerThread();

    boost::asio::io_context& _pIoContext;
    std::unique_ptr<grpc::Server> _pGrpcServer;
    std::atomic<bool> _pIsRunning{false};
    std::atomic<size_t> _pActiveConnections{0};
    std::string _pEndpoint;
    std::unique_ptr<std::thread> _pGrpcThread;
    std::unique_ptr<class FKTokenServiceImpl> _pService;
};

// gRPC服务实现类
class FKTokenServiceImpl final : public im::service::TokenService::Service {
public:
    FKTokenServiceImpl();
    ~FKTokenServiceImpl();

    // 启动和停止清理任务
    void startCleanupTask();
    void stopCleanupTask();

    // 生成Token并分配聊天服务器
    grpc::Status GenerateToken(grpc::ServerContext* context,
        const im::service::GenerateTokenRequest* request,
        im::service::GenerateTokenResponse* response) override;

    // 验证Token有效性
    grpc::Status ValidateToken(grpc::ServerContext* context,
        const im::service::ValidateTokenRequest* request,
        im::service::ValidateTokenResponse* response) override;

    //// 撤销Token（用户登出时调用）
    //grpc::Status RevokeToken(grpc::ServerContext* context,
    //    const im::service::RevokeTokenRequest* request,
    //    im::service::RevokeTokenResponse* response) override;

private:
    struct ChatServerStatus {
        std::string id;
        std::string host;
        int32_t port;
        std::atomic<int32_t> current_load{0};
        int32_t max_connections;
        std::atomic<bool> is_active{true};
        std::string zone;
    };

    // JWT相关方法
    std::string _generateJWT(const std::string& user_uuid, const std::string& client_device_id);
    bool _validateJWT(const std::string& token, std::string& user_uuid, std::string& client_device_id, int64_t& expires_at);
    
    // 聊天服务器负载均衡
    im::service::ChatServerInfo _selectBestChatServer();
    void _updateServerLoad(const std::string& server_id, int32_t load);
    void _decrementServerLoad(const std::string& server_id);
    
    // 聊天服务器列表管理
    std::vector<std::shared_ptr<ChatServerStatus>> _pChatServers;
    mutable std::shared_mutex _pServersMutex; // 使用读写锁提高并发性能
    
    // JWT密钥
    std::string _pJwtSecret;
    
    // 清理任务相关
    std::unique_ptr<std::thread> _pCleanupThread;
    std::atomic<bool> _pCleanupRunning{false};
    void _cleanupTask();
};

#endif // FK_STATUS_SERVER_H_