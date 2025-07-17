#include "FKGrpcServicePoolManager.h"
#include <magic_enum/magic_enum.hpp>

#include "FKLogger.h"
#include "Source/FKConfigManager.h"

SINGLETON_CREATE_SHARED_CPP(FKGrpcServicePoolManager)

FKGrpcServicePoolManager::FKGrpcServicePoolManager() {
    // 记录初始化失败的服务，但继续尝试初始化其他服务
    std::vector<std::string> failedServices;
    
    try {
        _initializeService(flicker::grpc::service::VerifyCode);
    } catch (const std::exception& e) {
        failedServices.push_back(std::string("验证码服务: ") + e.what());
    }
    
    try {
        _initializeService(flicker::grpc::service::EncryptPassword);
    } catch (const std::exception& e) {
        failedServices.push_back(std::string("密码加密服务: ") + e.what());
    }
    
    try {
        _initializeService(flicker::grpc::service::AuthenticatePwdReset);
    } catch (const std::exception& e) {
        failedServices.push_back(std::string("密码重置验证服务: ") + e.what());
    }
    
    // 如果有服务初始化失败，记录错误信息
    if (!failedServices.empty()) {
        std::string errorMsg = "以下服务初始化失败:\n";
        for (const auto& service : failedServices) {
            errorMsg += "- " + service + "\n";
        }
        FK_SERVER_ERROR(errorMsg);
    }
}

FKGrpcServicePoolManager::~FKGrpcServicePoolManager() {
    shutdownAllServices();
}

void FKGrpcServicePoolManager::_initializeService(flicker::grpc::service rpcService) {
#define GRPC_SERVICE_CASE(type, name)  case type: { \
        using service_type = ServiceTraits<type>::Type; \
        try { \
            std::unique_ptr<FKGrpcConnectionPool<service_type>> pool( \
                new FKGrpcConnectionPool<service_type>(configManager->getGrpcServiceConfig(type)) \
            ); \
            auto rawPool = pool.release(); \
            _pServicePools[type] = { \
               .poolPtr = rawPool, \
               .shutdown = [rawPool] { \
                    rawPool->shutdown(); \
                    delete rawPool; \
                }, \
               .getStatus = [rawPool] { \
                    return rawPool->getStatus(); \
                } \
            }; \
            FK_SERVER_INFO(std::format("已初始化{}服务连接池", name));\
        } catch (const std::exception& e) { \
            FK_SERVER_ERROR(std::format("初始化{}服务连接池失败: {}", name, e.what())); \
            throw; \
        } \
        break; \
    }

    std::lock_guard<std::mutex> lock(_pMutex);
    if (_pServicePools.find(rpcService) != _pServicePools.end()) {
        FK_SERVER_WARN("服务已初始化，请先关闭再重新初始化");
        return;
    }

    FKConfigManager* configManager = FKConfigManager::getInstance();
    try {
        switch (rpcService) {
            GRPC_SERVICE_CASE(flicker::grpc::service::VerifyCode, "验证码")
            GRPC_SERVICE_CASE(flicker::grpc::service::EncryptPassword, "加密密码")
            GRPC_SERVICE_CASE(flicker::grpc::service::AuthenticatePwdReset, "验证密码重置")
            default: throw std::runtime_error("未知的服务类型");
        }
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("初始化{}服务失败: {}", magic_enum::enum_name(rpcService), e.what()));
        throw;
    }
#undef GRPC_SERVICE_CASE
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
