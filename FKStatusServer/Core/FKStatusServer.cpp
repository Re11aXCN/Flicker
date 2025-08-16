#include "FKStatusServer.h"

#include <jwt-cpp/jwt.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <format>

#include "universal/utils.h"
#include "Library/Logger/logger.h"
#include "Flicker/Global/Redis/FKRedisSingleton.h"

FKStatusServer::FKStatusServer(boost::asio::io_context& ioc, std::string&& endpoint)
    : _pIoContext(ioc), _pEndpoint(std::move(endpoint))
{
}

FKStatusServer::~FKStatusServer()
{
    stop();
}

void FKStatusServer::start()
{
    if (_pIsRunning.exchange(true)) {
        LOGGER_WARN("状态服务器已经在运行中");
        return;
    }
    
    try {
        _pService = std::make_unique<FKTokenServiceImpl>();
        
        grpc::ServerBuilder builder;
        builder.AddListeningPort(_pEndpoint, grpc::InsecureServerCredentials());
        builder.RegisterService(_pService.get()); // 注册服务实现
        
        // 配置gRPC服务器参数以防止too_many_pings错误
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);  // 30秒
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000); // 10秒
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 0); // 禁止无调用时发送keepalive
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0); // 无数据时不允许ping
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 300000); // 5分钟
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 300000); // 5分钟
        builder.AddChannelArgument(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 10000);
        builder.AddChannelArgument(GRPC_ARG_GRPCLB_CALL_TIMEOUT_MS, 5000);
        
        // 启动服务器
        _pGrpcServer = builder.BuildAndStart();
        
        if (_pGrpcServer) {
            LOGGER_INFO(std::format("状态服务器启动成功，监听端点: {}", _pEndpoint));
            
            // 在管理的线程中运行gRPC服务器
            _pGrpcThread = std::make_unique<std::thread>(&FKStatusServer::_grpcServerThread, this);
        } else {
            LOGGER_ERROR("状态服务器启动失败");
            _pIsRunning = false;
            _pService.reset();
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("状态服务器启动异常: {}", e.what()));
        _pIsRunning = false;
        _pService.reset();
    }
}

void FKStatusServer::stop()
{
    if (!_pIsRunning.exchange(false)) {
        return;
    }
    
    LOGGER_INFO("状态服务器正在停止...");
    
    // 首先关闭gRPC服务器
    if (_pGrpcServer) {
        _pGrpcServer->Shutdown();
    }
    
    // 等待gRPC线程结束
    if (_pGrpcThread && _pGrpcThread->joinable()) {
        _pGrpcThread->join();
        _pGrpcThread.reset();
    }
    
    // 清理资源
    _pGrpcServer.reset();
    _pService.reset();
    
    LOGGER_INFO("状态服务器已停止");
}

void FKStatusServer::_grpcServerThread()
{
    try {
        if (_pGrpcServer) {
            _pGrpcServer->Wait();
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("gRPC服务器线程异常: {}", e.what()));
    }
    
    LOGGER_INFO("gRPC服务器线程已退出");
}

void FKStatusServer::_incrementActiveConnections()
{
    ++_pActiveConnections;
}

void FKStatusServer::_decrementActiveConnections()
{
    --_pActiveConnections;
}

// ============================================================================
// FKTokenServiceImpl 实现
// ============================================================================

FKTokenServiceImpl::FKTokenServiceImpl()
    : _pJwtSecret("flicker_jwt_secret_key_2024")
{
    // 初始化聊天服务器列表（两个服务器用于负载均衡）
    auto serverMaster = std::make_shared<ChatServerStatus>();
    Flicker::Server::Config::ChatMasterServer master_config;
    Flicker::Server::Config::ChatSlaveServer slave_config;
    serverMaster->id = master_config.ID;
    serverMaster->host = master_config.Host;
    serverMaster->port = master_config.Port;
    serverMaster->current_load = 0;
    serverMaster->max_connections = 1000;
    serverMaster->is_active = true;
    serverMaster->zone = universal::utils::time::get_timezone_offset();

    auto serverSlave = std::make_shared<ChatServerStatus>();
    serverSlave->id = slave_config.ID;
    serverSlave->host = slave_config.Host;
    serverSlave->port = slave_config.Port;
    serverSlave->current_load = 0;
    serverSlave->max_connections = 1000;
    serverSlave->is_active = true;
    serverSlave->zone = universal::utils::time::get_timezone_offset();

    _pChatServers.push_back(serverMaster);
    _pChatServers.push_back(serverSlave);
    
    LOGGER_INFO("TokenService初始化完成，聊天服务器数量: {}", _pChatServers.size());
    
    // 启动清理任务
    startCleanupTask();
}

FKTokenServiceImpl::~FKTokenServiceImpl()
{
    stopCleanupTask();
}

void FKTokenServiceImpl::startCleanupTask()
{
    if (_pCleanupRunning.exchange(true)) {
        return; // 已经在运行
    }
    
    _pCleanupThread = std::make_unique<std::thread>(&FKTokenServiceImpl::_cleanupTask, this);
    LOGGER_INFO("Token清理任务已启动");
}

void FKTokenServiceImpl::stopCleanupTask()
{
    if (!_pCleanupRunning.exchange(false)) {
        return; // 已经停止
    }
    
    if (_pCleanupThread && _pCleanupThread->joinable()) {
        _pCleanupThread->join();
        _pCleanupThread.reset();
    }
    
    LOGGER_INFO("Token清理任务已停止");
}

void FKTokenServiceImpl::_cleanupTask()
{
    try {
        while (_pCleanupRunning.load()) {
            // 每30分钟清理一次过期Token
            for (int i = 0; i < 1800 && _pCleanupRunning.load(); ++i) { // 30分钟 = 1800秒
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            if (!_pCleanupRunning.load()) {
                break;
            }
            
            // 执行清理
            auto cleanup_result = FKRedisSingleton::cleanupExpiredTokens();
            if (cleanup_result) {
                if (*cleanup_result > 0) {
                    LOGGER_INFO(std::format("清理了 {} 个过期Token", *cleanup_result));
                }
            } else {
                LOGGER_WARN(std::format("Token清理失败: {}", cleanup_result.error().message));
            }
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("Token清理任务异常: {}", e.what()));
    }
    
    LOGGER_INFO("Token清理任务线程已退出");
}

grpc::Status FKTokenServiceImpl::GenerateToken(grpc::ServerContext* context,
                                              const im::service::GenerateTokenRequest* request,
                                              im::service::GenerateTokenResponse* response)
{
    try {
        LOGGER_INFO(std::format("收到生成Token请求，用户: {}, 设备: {}", 
                    request->user_uuid(), request->client_device_id()));
        
        // 输入验证
        if (request->user_uuid().empty()) {
            response->set_status(im::service::StatusCode::bad_request);
            response->set_error_detail("User UUID cannot be empty");
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "User UUID cannot be empty");
        }
        
        if (request->client_device_id().empty()) {
            response->set_status(im::service::StatusCode::bad_request);
            response->set_error_detail("Device ID cannot be empty");
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Device ID cannot be empty");
        }
        
        // 生成JWT Token
        std::string token = _generateJWT(request->user_uuid(), request->client_device_id());
        
        // 选择最佳聊天服务器
        im::service::ChatServerInfo chat_server = _selectBestChatServer();
        
        // 设置响应
        response->set_status(im::service::StatusCode::ok);
        response->set_token(token);
        
        // 设置过期时间（24小时后）
        auto expires_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() + (24 * 60 * 60 * 1000);
        response->set_expires_at(expires_at);
        
        // 设置聊天服务器信息
        response->mutable_chat_server_info()->CopyFrom(chat_server);
        
        // 存储Token到Redis
        auto store_result = FKRedisSingleton::storeToken(token, request->user_uuid());
        if (!store_result) {
            LOGGER_ERROR(std::format("Token存储失败: {}", store_result.error().message));
            response->set_status(im::service::StatusCode::internal_server_error);
            response->set_error_detail("Failed to store token");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to store token");
        }
        
        LOGGER_INFO(std::format("Token生成成功，分配聊天服务器: {}:{}", 
                    chat_server.host(), chat_server.port()));
        
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("生成Token异常: {}", e.what()));
        response->set_status(im::service::StatusCode::internal_server_error);
        response->set_error_detail(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status FKTokenServiceImpl::ValidateToken(grpc::ServerContext* context,
                                              const im::service::ValidateTokenRequest* request,
                                              im::service::ValidateTokenResponse* response)
{
    try {
        LOGGER_INFO(std::format("收到验证Token请求，设备: {}", request->client_device_id()));
        
        // 输入验证
        if (request->token().empty()) {
            response->set_status(im::service::StatusCode::bad_request);
            response->set_error_detail("Token cannot be empty");
            return grpc::Status::OK;
        }
        
        std::string user_uuid;
        std::string client_device_id;
        int64_t expires_at;
        
        // 首先验证JWT格式、是否过期和签名
        if (!_validateJWT(request->token(), user_uuid, client_device_id, expires_at)) {
            response->set_status(im::service::StatusCode::unauthorized);
            response->set_error_detail("Invalid JWT token");
            LOGGER_WARN("JWT验证失败");
            return grpc::Status::OK;
        }

        // 验证设备ID
        if (client_device_id != request->client_device_id()) {
            response->set_status(im::service::StatusCode::unauthorized);
            response->set_error_detail("Device ID mismatch");
            LOGGER_WARN("设备ID不匹配");
            return grpc::Status::OK;
        }
        
        // 然后验证Token是否在Redis中存在（防止JWT验证绕过）
        auto redis_user = FKRedisSingleton::getTokenUser(request->token());
        if (!redis_user) {
            response->set_status(im::service::StatusCode::unauthorized);
            response->set_error_detail("Token not found or expired");
            LOGGER_WARN(std::format("Token在Redis中不存在或已过期: {}", redis_user.error().message));
            return grpc::Status::OK;
        }
        
        // 验证Redis中的用户UUID与JWT中的是否一致
        if (*redis_user != user_uuid) {
            response->set_status(im::service::StatusCode::unauthorized);
            response->set_error_detail("Token user mismatch");
            LOGGER_WARN(std::format("Token用户不匹配，JWT: {}, Redis: {}", user_uuid, *redis_user));
            return grpc::Status::OK;
        }
        
        response->set_status(im::service::StatusCode::ok);
        response->set_user_uuid(user_uuid);
        response->set_expires_at(expires_at);
        
        LOGGER_INFO(std::format("Token验证成功，用户: {}", user_uuid));
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("验证Token异常: {}", e.what()));
        response->set_status(im::service::StatusCode::internal_server_error);
        response->set_error_detail(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

//grpc::Status FKTokenServiceImpl::RevokeToken(grpc::ServerContext* context,
//                                            const im::service::RevokeTokenRequest* request,
//                                            im::service::RevokeTokenResponse* response)
//{
//    try {
//        LOGGER_INFO(std::format("收到撤销Token请求，设备: {}", request->client_device_id()));
//        
//        // 输入验证
//        if (request->token().empty()) {
//            response->set_status(im::service::StatusCode::bad_request);
//            response->set_error_detail("Token cannot be empty");
//            return grpc::Status::OK;
//        }
//        
//        // 从Redis中删除Token
//        auto delete_result = FKRedisSingleton::deleteToken(request->token());
//        if (!delete_result) {
//            if (delete_result.error().code == RedisErrorCode::KeyNotFound) {
//                // Token不存在，可能已经过期，这不是错误
//                response->set_status(im::service::StatusCode::ok);
//                response->set_message("Token already expired or not found");
//                LOGGER_INFO("Token已过期或不存在");
//                return grpc::Status::OK;
//            } else {
//                LOGGER_ERROR(std::format("Token删除失败: {}", delete_result.error().message));
//                response->set_status(im::service::StatusCode::internal_server_error);
//                response->set_error_detail("Failed to revoke token");
//                return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to revoke token");
//            }
//        }
//        
//        // 如果提供了服务器ID，减少该服务器的负载
//        if (!request->server_id().empty()) {
//            _decrementServerLoad(request->server_id());
//            LOGGER_DEBUG(std::format("减少服务器 {} 的负载", request->server_id()));
//        }
//        
//        response->set_status(im::service::StatusCode::ok);
//        response->set_message("Token revoked successfully");
//        
//        LOGGER_INFO("Token撤销成功");
//        return grpc::Status::OK;
//    }
//    catch (const std::exception& e) {
//        LOGGER_ERROR(std::format("撤销Token异常: {}", e.what()));
//        response->set_status(im::service::StatusCode::internal_server_error);
//        response->set_error_detail(e.what());
//        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
//    }
//}

std::string FKTokenServiceImpl::_generateJWT(const std::string& user_uuid, const std::string& client_device_id)
{
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(24); // 24小时过期
    
    auto token = jwt::create()
        .set_issuer("flicker_state_server")
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(expires)
        .set_payload_claim("user_uuid", jwt::claim(user_uuid))
        .set_payload_claim("client_device_id", jwt::claim(client_device_id))
        .sign(jwt::algorithm::hs256{_pJwtSecret});
    
    return token;
}

bool FKTokenServiceImpl::_validateJWT(const std::string& token, std::string& user_uuid, std::string& client_device_id, int64_t& expires_at)
{
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{_pJwtSecret})
            .with_issuer("flicker_state_server");
        
        auto decoded = jwt::decode(token);
        verifier.verify(decoded);
        
        //// 检查是否过期,verify() 已自动验证过期时间
        auto exp = decoded.get_expires_at();
        //auto now = std::chrono::system_clock::now();
        //if (exp < now) {
        //    return false;
        //}
        
        user_uuid = decoded.get_payload_claim("user_uuid").as_string();
        client_device_id = decoded.get_payload_claim("client_device_id").as_string();
        expires_at = std::chrono::duration_cast<std::chrono::milliseconds>(exp.time_since_epoch()).count();
        
        return true;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("JWT验证异常: {}", e.what()));
        return false;
    }
}

im::service::ChatServerInfo FKTokenServiceImpl::_selectBestChatServer()
{
    std::shared_lock<std::shared_mutex> read_lock(_pServersMutex);
    
    // 边界检查
    if (_pChatServers.empty()) {
        LOGGER_ERROR("没有可用的聊天服务器");
        return im::service::ChatServerInfo(); // 返回空的服务器信息
    }
    
    // 选择负载最低的活跃服务器
    std::shared_ptr<ChatServerStatus> best_server = nullptr;
    int32_t min_load = INT32_MAX;
    
    for (auto& server : _pChatServers) {
        if (!server) {
            LOGGER_WARN("发现空的服务器指针，跳过");
            continue;
        }
        
        int32_t current_load = server->current_load.load();
        if (server->is_active.load() && current_load < min_load && current_load < server->max_connections) {
            min_load = current_load;
            best_server = server;
        }
    }
    
    // 如果没有找到活跃服务器，使用第一个非空服务器
    if (!best_server) {
        for (auto& server : _pChatServers) {
            if (server) {
                best_server = server;
                LOGGER_WARN(std::format("没有活跃服务器，使用备用服务器: {}:{}", 
                           server->host, server->port));
                break;
            }
        }
    }
    
    im::service::ChatServerInfo chat_server;
    if (best_server) {
        chat_server.set_id(best_server->id);
        chat_server.set_zone(best_server->zone);
        chat_server.set_host(best_server->host);
        chat_server.set_port(best_server->port);
        chat_server.set_current_load(best_server->current_load.load());
        chat_server.set_max_connections(best_server->max_connections);
        
        // 原子地增加负载计数
        int32_t new_load = best_server->current_load.fetch_add(1) + 1;
        LOGGER_INFO(std::format("选择服务器 {}:{}, 当前负载: {}/{}", 
                    best_server->host, best_server->port, 
                    new_load, best_server->max_connections));
    } else {
        LOGGER_ERROR("没有可用的聊天服务器");
    }
    
    return chat_server;
}

void FKTokenServiceImpl::_updateServerLoad(const std::string& server_id, int32_t load)
{
    if (server_id.empty()) {
        LOGGER_WARN("服务器ID为空，无法更新负载");
        return;
    }
    
    if (load < 0) {
        LOGGER_WARN(std::format("负载值无效: {}, 设置为0", load));
        load = 0;
    }
    
    std::shared_lock<std::shared_mutex> read_lock(_pServersMutex);
    
    bool found = false;
    for (auto& server : _pChatServers) {
        if (server && server->id == server_id) {
            server->current_load.store(load);
            found = true;
            LOGGER_DEBUG(std::format("更新服务器 {} 负载为: {}", server_id, load));
            break;
        }
    }
    
    if (!found) {
        LOGGER_WARN(std::format("未找到服务器: {}", server_id));
    }
}

void FKTokenServiceImpl::_decrementServerLoad(const std::string& server_id)
{
    if (server_id.empty()) {
        LOGGER_WARN("服务器ID为空，无法减少负载");
        return;
    }
    
    std::shared_lock<std::shared_mutex> read_lock(_pServersMutex);
    
    bool found = false;
    for (auto& server : _pChatServers) {
        if (server && server->id == server_id) {
            // 原子地减少负载计数，确保不会小于0
            int32_t current = server->current_load.load();
            while (current > 0 && !server->current_load.compare_exchange_weak(current, current - 1)) {
                // CAS失败时重试
            }
            found = true;
            LOGGER_DEBUG(std::format("减少服务器 {} 负载，当前负载: {}", server_id, server->current_load.load()));
            break;
        }
    }
    
    if (!found) {
        LOGGER_WARN(std::format("未找到服务器: {}", server_id));
    }
}