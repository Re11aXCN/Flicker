/*************************************************************************************
 *
 * @ Filename     : GrpcServicePoolManager.h
 * @ Description : gRPC服务管理器，管理不同的gRPC业务连接池
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
#ifndef GRPC_SERVICE_POOL_MANAGER_H_
#define GRPC_SERVICE_POOL_MANAGER_H_

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

#include "Common/global/define_enum.h"
#include "Common/global/macro.h"
#include "Common/config/struct_config.h"
#include "Common/grpc/grpc_connection_pool.hpp"

#pragma region SERVICE_TRAITS_TEMPLATE
#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4267)
#include "grpc_service.grpc.pb.h"
#pragma warning(pop)

// 服务特性模板
template <flicker::grpc::service T> struct ServiceTraits;

// 特化模板
template<> struct ServiceTraits<flicker::grpc::service::VerifyCode> {
    using Type = GrpcService::Verification;
};

template<> struct ServiceTraits<flicker::grpc::service::EncryptPassword> {
    using Type = GrpcService::Encryption;
};

template<> struct ServiceTraits<flicker::grpc::service::AuthenticatePwdReset> {
    using Type = GrpcService::Authentication;
};
template<> struct ServiceTraits<flicker::grpc::service::GetChatServerAddress> {
    using Type = GrpcService::Interaction;
};
#pragma endregion SERVICE_TRAITS_TEMPLATE

class GrpcServicePoolManager {
    SINGLETON_CREATE_SHARED_H(GrpcServicePoolManager)
public:
    // 获取所有服务的状态信息
    std::string getAllServicesStatus() const;
    void shutdownService(flicker::grpc::service rpcService);
    void shutdownAllServices();

    // 获取特定服务类型的连接池，可以链式调用特定连接池提供服务的接口
    template<flicker::grpc::service rpcService>
    auto& getServicePool() {
        std::lock_guard<std::mutex> lock(_pMutex);
        
        auto it = _pServicePools.find(rpcService);
        if (it == _pServicePools.end()) {
            throw std::runtime_error(std::format("服务未初始化: {}", static_cast<int>(rpcService)));
        }
        
        // 使用static_cast将void*转换为具体类型
        using ServiceTraitType = typename ServiceTraits<rpcService>::Type;
        auto* pool = static_cast<GrpcConnectionPool<ServiceTraitType>*>(it->second.poolPtr);
        return *pool;
    }
    
private:
    GrpcServicePoolManager();
    ~GrpcServicePoolManager();
    // 初始化特定类型的gRPC服务连接池
    template<flicker::grpc::service rpcService>
    void _initializeService();

    struct ServicePoolWrapper {
        void* poolPtr{ nullptr };
        std::function<void()> shutdown;
        std::function<std::string()> getStatus;
    };
    std::unordered_map<flicker::grpc::service, ServicePoolWrapper> _pServicePools;
    mutable std::mutex _pMutex;
};


#endif // !GRPC_SERVICE_POOL_MANAGER_H_