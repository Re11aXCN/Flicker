/*************************************************************************************
 *
 * @ Filename     : FKGrpcServiceStubPoolManager.h
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
#ifndef FK_GRPC_SERVICE_STUB_POOL_MANAGER_H_
#define FK_GRPC_SERVICE_STUB_POOL_MANAGER_H_

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

#include "FKDef.h"
#include "FKGrpcServiceStubPool.hpp"

#pragma region SERVICE_TRAITS_TEMPLATE
#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4267)
#include "FKGrpcService.grpc.pb.h"
#pragma warning(pop)

// 服务特性模板
template <Flicker::Server::Enums::GrpcServiceType T> struct ServiceTraits;

// 特化模板
template<> struct ServiceTraits<Flicker::Server::Enums::GrpcServiceType::AuthenticateLogin> {
    using Type = im::service::AuthenticationService;
};

template<> struct ServiceTraits<Flicker::Server::Enums::GrpcServiceType::GenerateToken> {
    using Type = im::service::TokenService;
};

template<> struct ServiceTraits<Flicker::Server::Enums::GrpcServiceType::ValidateToken> {
    using Type = im::service::TokenService;
};
#pragma endregion SERVICE_TRAITS_TEMPLATE

class FKGrpcServiceStubPoolManager {
    SINGLETON_CREATE_SHARED_H(FKGrpcServiceStubPoolManager)
public:
    // 初始化特定类型的gRPC服务连接池
    template<Flicker::Server::Enums::GrpcServiceType rpcService>
    void initializeService(const Flicker::Server::Config::BaseGrpcService& config){
        if (_pServicePools.contains(rpcService)) {
            LOGGER_WARN("服务已初始化，请先关闭再重新初始化");
            return;
        }
        try {
            using service_type = ServiceTraits<rpcService>::Type;
            std::unique_ptr<FKGrpcServiceStubPool<service_type>> pool{ new FKGrpcServiceStubPool<service_type>(config) };
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


    // 获取所有服务的状态信息
    std::string getAllServicesStatus() const;
    void shutdownService(Flicker::Server::Enums::GrpcServiceType rpcService);
    void shutdownAllServices();

    // 获取特定服务类型的连接池，可以链式调用特定连接池提供服务的接口
    template<Flicker::Server::Enums::GrpcServiceType rpcService>
    auto& getServicePool() {
        std::lock_guard<std::mutex> lock(_pMutex);
        
        auto it = _pServicePools.find(rpcService);
        if (it == _pServicePools.end()) {
            throw std::runtime_error(std::format("服务未初始化: {}", static_cast<int>(rpcService)));
        }
        
        // 使用static_cast将void*转换为具体类型
        using ServiceTraitType = typename ServiceTraits<rpcService>::Type;
        auto* pool = static_cast<FKGrpcServiceStubPool<ServiceTraitType>*>(it->second.poolPtr);
        return *pool;
    }
    
private:
    FKGrpcServiceStubPoolManager();
    ~FKGrpcServiceStubPoolManager();

    struct ServicePoolWrapper {
        void* poolPtr{ nullptr };
        std::function<void()> shutdown;
        std::function<std::string()> getStatus;
    };
    std::unordered_map<Flicker::Server::Enums::GrpcServiceType, ServicePoolWrapper> _pServicePools;
    mutable std::mutex _pMutex;
};


#endif // !FK_GRPC_SERVICE_STUB_POOL_MANAGER_H_