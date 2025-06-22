#include "FKGrpcServiceManager.h"
#include "../FKServerConfig.h"
#include <magic_enum/magic_enum.hpp>

SINGLETON_CREATE_SHARED_CPP(FKGrpcServiceManager)

FKGrpcServiceManager::FKGrpcServiceManager() {
    std::println("FKGrpcServiceManager 已创建");
    
    // 从配置加载gRPC服务配置
    FKServerConfig* config = FKServerConfig::getInstance();
    
    // 初始化验证码服务
    try {
        auto verifyCodeConfig = config->getGrpcServiceConfig(gRPC::ServiceType::VERIFY_CODE_SERVICE);
        _initializeService(gRPC::ServiceType::VERIFY_CODE_SERVICE, std::move(verifyCodeConfig));
    } catch (const std::exception& e) {
        std::println("初始化验证码服务失败: {}", e.what());
    }
}

FKGrpcServiceManager::~FKGrpcServiceManager() {
    shutdownAllServices();
}

void FKGrpcServiceManager::_initializeService(gRPC::ServiceType serviceType, FKGrpcServiceConfig&& config) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    // 检查服务是否已存在
    if (_pServicePools.find(serviceType) != _pServicePools.end()) {
        std::println("服务已初始化，请先关闭再重新初始化");
        return;
    }
    
    try {
        switch (serviceType) {
            case gRPC::ServiceType::VERIFY_CODE_SERVICE: {
				// 使用unique_ptr确保异常安全
				std::unique_ptr<FKGrpcConnectionPool<ServiceTraits<gRPC::ServiceType::VERIFY_CODE_SERVICE>::Type>> pool(
					new FKGrpcConnectionPool<ServiceTraits<gRPC::ServiceType::VERIFY_CODE_SERVICE>::Type>()
				);
                pool->initialize(std::move(config));

				auto rawPool = pool.release();
				_pServicePools[serviceType] = {
					.poolPtr = rawPool,
					.shutdown = [rawPool] {
						rawPool->shutdown();
						delete rawPool; // 释放内存
					},
					.getStatus = [rawPool] {
						return rawPool->getStatus();
					}
				};
                std::println("已初始化验证码服务连接池");
                break;
            }
            case gRPC::ServiceType::USER_AUTH_SERVICE: {
                // 实际实现时添加
                std::println("用户认证服务尚未实现");
                break;
            }
            case gRPC::ServiceType::PROFILE_SERVICE: {
                // 实际实现时添加
                std::println("用户资料服务尚未实现");
                break;
            }
            case gRPC::ServiceType::MESSAGE_SERVICE: {
                // 实际实现时添加
                std::println("消息服务尚未实现");
                break;
            }
            default:
                throw std::runtime_error("未知的服务类型");
        }
    }
    catch (const std::exception& e) {
        std::println("初始化服务失败: {}", e.what());
        throw;
    }
}

void FKGrpcServiceManager::shutdownService(gRPC::ServiceType serviceType) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    auto it = _pServicePools.find(serviceType);
    if (it != _pServicePools.end()) {
		// 先关闭连接池
		it->second.shutdown();
		// 然后移除
		_pServicePools.erase(it);
        std::println("已关闭服务连接池: {}", static_cast<int>(serviceType));
    }
}

void FKGrpcServiceManager::shutdownAllServices() {
    std::lock_guard<std::mutex> lock(_pMutex);
	for (auto& [type, wrapper] : _pServicePools) {
		wrapper.shutdown();
	}
    _pServicePools.clear();
    std::println("已关闭所有服务连接池");
}

std::string FKGrpcServiceManager::getAllServicesStatus() const {
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
