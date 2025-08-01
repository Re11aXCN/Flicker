#include "Common/grpc/grpc_service_pool_manager.h"

#include <sstream>
#include <magic_enum/magic_enum.hpp>

#include "Common/config/config_manager.h"

SINGLETON_CREATE_SHARED_CPP(GrpcServicePoolManager)

GrpcServicePoolManager::GrpcServicePoolManager() {
    _initializeService<flicker::grpc::service::VerifyCode>();
    _initializeService<flicker::grpc::service::EncryptPassword>();
    _initializeService<flicker::grpc::service::AuthenticatePwdReset>();
	_initializeService<flicker::grpc::service::GetChatServerAddress>();
}

GrpcServicePoolManager::~GrpcServicePoolManager() {
    shutdownAllServices();
}

template<flicker::grpc::service rpcService>
void GrpcServicePoolManager::_initializeService() {
    if (_pServicePools.contains(rpcService)) {
        LOGGER_WARN("服务已初始化，请先关闭再重新初始化");
        return;
    }
    try {
        ConfigManager* configManager = ConfigManager::getInstance();

        using service_type = ServiceTraits<rpcService>::Type;
        std::unique_ptr<GrpcConnectionPool<service_type>> pool{ new GrpcConnectionPool<service_type>(configManager->getGrpcServiceConfig(rpcService)) };
        auto rawPool = pool.release();
        _pServicePools[rpcService] = {
            .poolPtr = rawPool,
            .shutdown = [rawPool] { rawPool->shutdown(); delete rawPool; },
            .getStatus = [rawPool] { return rawPool->getStatus(); }
        };
        LOGGER_INFO(std::format("已初始化{}服务连接池", magic_enum::enum_name(rpcService)));
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("初始化{}服务失败: {}", magic_enum::enum_name(rpcService), e.what()));
        throw;
    }
}

void GrpcServicePoolManager::shutdownService(flicker::grpc::service rpcService) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    auto it = _pServicePools.find(rpcService);
    if (it != _pServicePools.end()) {
        // 先关闭连接池
        it->second.shutdown();
        // 然后移除
        _pServicePools.erase(it);
        LOGGER_INFO(std::format("已关闭{}服务连接池", magic_enum::enum_name(rpcService)));
    }
}

void GrpcServicePoolManager::shutdownAllServices() {
    std::lock_guard<std::mutex> lock(_pMutex);
    for (auto& [type, wrapper] : _pServicePools) {
        wrapper.shutdown();
    }
    _pServicePools.clear();
    LOGGER_INFO("已关闭所有服务连接池");
}

std::string GrpcServicePoolManager::getAllServicesStatus() const {
    std::lock_guard<std::mutex> lock(_pMutex);
    std::ostringstream oss;

    oss << "gRPC Services Status:\n";
    oss << "================================\n";

    for (const auto& [type, wrapper] : _pServicePools) {
        oss << "[" << magic_enum::enum_name(type) << "]: "
            << wrapper.getStatus() << "\n";
    }

    if (_pServicePools.empty()) {
        oss << "No active services\n";
    }

    oss << "================================\n";
    return oss.str();
}
