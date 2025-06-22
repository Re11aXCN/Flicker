/*************************************************************************************
 *
 * @ Filename	 : FKGrpcServiceManager.h
 * @ Description : gRPC服务管理器，管理不同的gRPC业务连接池
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
#ifndef FK_GRPC_SERVICE_MANAGER_H_
#define FK_GRPC_SERVICE_MANAGER_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <print>

#include "FKDef.h"
#include "FKMacro.h"
#include "FKGrpcConnectionPool.hpp"
#include "../FKConfigManager.h"

#pragma region SERVICE_TRAITS_TEMPLATE
#include "FKVerifyGrpc.grpc.pb.h"
// 服务特性模板
template <gRPC::ServiceType T> struct ServiceTraits;

// 特化模板
template<> struct ServiceTraits<gRPC::ServiceType::VERIFY_CODE_SERVICE> {
	using Type = FKVerifyGrpc::VarifyCodeService;
};
#pragma endregion SERVICE_TRAITS_TEMPLATE

class FKGrpcServiceManager {
    SINGLETON_CREATE_SHARED_H(FKGrpcServiceManager)
public:
    // 获取所有服务的状态信息
    std::string getAllServicesStatus() const;
    void shutdownService(gRPC::ServiceType serviceType);
    void shutdownAllServices();

    // 获取特定服务类型的连接池，可以链式调用特定连接池提供服务的接口
    template<gRPC::ServiceType ServiceType>
    auto& getServicePool() {
        std::lock_guard<std::mutex> lock(_pMutex);
        
        auto it = _pServicePools.find(ServiceType);
        if (it == _pServicePools.end()) {
            throw std::runtime_error(std::format("服务未初始化: {}", static_cast<int>(ServiceType)));
        }
        
        // 使用static_cast将void*转换为具体类型
        using ServiceTraitType = typename ServiceTraits<ServiceType>::Type;
        auto* pool = static_cast<FKGrpcConnectionPool<ServiceTraitType>*>(it->second.poolPtr);
        return *pool;
    }
    
private:
    FKGrpcServiceManager();
    ~FKGrpcServiceManager();
	// 初始化特定类型的gRPC服务连接池
	void _initializeService(gRPC::ServiceType serviceType, const FKGrpcServiceConfig& config);

	struct ServicePoolWrapper {
		void* poolPtr{ nullptr };
		std::function<void()> shutdown;
		std::function<std::string()> getStatus;
	};
    std::unordered_map<gRPC::ServiceType, ServicePoolWrapper> _pServicePools;
    mutable std::mutex _pMutex;
};


#endif // !FK_GRPC_SERVICE_MANAGER_H_