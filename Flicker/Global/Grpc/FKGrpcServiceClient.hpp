#ifndef FK_GRPC_SERVICE_CLIENT_H_
#define FK_GRPC_SERVICE_CLIENT_H_

#include "FKDef.h"
#include "FKGrpcServiceStubPoolManager.h"

template<Flicker::Server::Enums::GrpcServiceType T>
class FKGrpcServiceClient {
public:
    FKGrpcServiceClient() = default;
    ~FKGrpcServiceClient() = default;
    FKGrpcServiceClient(const FKGrpcServiceClient&) = delete;
    FKGrpcServiceClient& operator=(const FKGrpcServiceClient&) = delete;
    FKGrpcServiceClient(FKGrpcServiceClient&&) = delete;
    FKGrpcServiceClient& operator=(FKGrpcServiceClient&&) = delete;
    auto authenticateLogin(const im::service::AuthenticateLoginRequest& request) {
        return FKGrpcServiceStubPoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            im::service::AuthenticateLoginResponse response;
            auto status = stub->AuthenticateLogin(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto generateToken(const im::service::GenerateTokenRequest& request) {
        return FKGrpcServiceStubPoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            im::service::GenerateTokenResponse response;
            auto status = stub->GenerateToken(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto validateToken(const im::service::ValidateTokenRequest& request) {
        return FKGrpcServiceStubPoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            im::service::ValidateTokenResponse response;
            auto status = stub->ValidateToken(&context, request, &response);
            return std::pair{ response,status };
            });
    }
};

#endif // !FK_GRPC_SERVICE_CLIENT_H_
