#include "FKGrpcServiceStubPoolManager.h"

#include <sstream>
#include <magic_enum/magic_enum.hpp>

SINGLETON_CREATE_SHARED_CPP(FKGrpcServiceStubPoolManager)

FKGrpcServiceStubPoolManager::FKGrpcServiceStubPoolManager() {
}

FKGrpcServiceStubPoolManager::~FKGrpcServiceStubPoolManager() {
    shutdownAllServices();
}

void FKGrpcServiceStubPoolManager::shutdownService(Flicker::Server::Enums::GrpcServiceType rpcService) {
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

void FKGrpcServiceStubPoolManager::shutdownAllServices() {
    std::lock_guard<std::mutex> lock(_pMutex);
    for (auto& [type, wrapper] : _pServicePools) {
        wrapper.shutdown();
    }
    _pServicePools.clear();
    LOGGER_INFO("已关闭所有服务连接池");
}

std::string FKGrpcServiceStubPoolManager::getAllServicesStatus() const {
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
