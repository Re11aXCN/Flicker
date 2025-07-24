#include "FKGrpcServicePoolManager.h"
#include <magic_enum/magic_enum.hpp>

#include "FKLogger.h"
#include "Source/FKConfigManager.h"

SINGLETON_CREATE_SHARED_CPP(FKGrpcServicePoolManager)

FKGrpcServicePoolManager::FKGrpcServicePoolManager() {
    _initializeService<flicker::grpc::service::VerifyCode>();
    _initializeService<flicker::grpc::service::EncryptPassword>();
    _initializeService<flicker::grpc::service::AuthenticatePwdReset>();
}

FKGrpcServicePoolManager::~FKGrpcServicePoolManager() {
    shutdownAllServices();
}

template<flicker::grpc::service rpcService>
void FKGrpcServicePoolManager::_initializeService() {
    if (_pServicePools.contains(rpcService)) {
        FK_SERVER_WARN("服务已初始化，请先关闭再重新初始化");
        return;
    }
    try {
        FKConfigManager* configManager = FKConfigManager::getInstance();

        using service_type = ServiceTraits<rpcService>::Type;
        std::unique_ptr<FKGrpcConnectionPool<service_type>> pool{ new FKGrpcConnectionPool<service_type>(configManager->getGrpcServiceConfig(rpcService)) };
        auto rawPool = pool.release();
        _pServicePools[rpcService] = {
            .poolPtr = rawPool,
            .shutdown = [rawPool] { rawPool->shutdown(); delete rawPool; },
            .getStatus = [rawPool] { return rawPool->getStatus(); }
        };
        FK_SERVER_INFO(std::format("已初始化{}服务连接池", magic_enum::enum_name(rpcService)));
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("初始化{}服务失败: {}", magic_enum::enum_name(rpcService), e.what()));
        throw;
    }
}

void FKGrpcServicePoolManager::shutdownService(flicker::grpc::service rpcService) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    auto it = _pServicePools.find(rpcService);
    if (it != _pServicePools.end()) {
        // 先关闭连接池
        it->second.shutdown();
        // 然后移除
        _pServicePools.erase(it);
        FK_SERVER_INFO(std::format("已关闭{}服务连接池", magic_enum::enum_name(rpcService)));
    }
}

void FKGrpcServicePoolManager::shutdownAllServices() {
    std::lock_guard<std::mutex> lock(_pMutex);
    for (auto& [type, wrapper] : _pServicePools) {
        wrapper.shutdown();
    }
    _pServicePools.clear();
    FK_SERVER_INFO("已关闭所有服务连接池");
}

std::string FKGrpcServicePoolManager::getAllServicesStatus() const {
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
