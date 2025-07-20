#ifndef FK_PASSWORD_GRPC_CLIENT_H_
#define FK_PASSWORD_GRPC_CLIENT_H_

#include "FKGrpcServicePoolManager.h"

template<flicker::grpc::service T>
class FKGrpcServiceClient {
public:
    FKGrpcServiceClient() = default;
    ~FKGrpcServiceClient() = default;
    FKGrpcServiceClient(const FKGrpcServiceClient&) = delete;
    FKGrpcServiceClient& operator=(const FKGrpcServiceClient&) = delete;
    FKGrpcServiceClient(FKGrpcServiceClient&&) = delete;
    FKGrpcServiceClient& operator=(FKGrpcServiceClient&&) = delete;

    auto getVerifyCode(const FKGrpcService::VerifyCodeRequestBody& request) {
        return FKGrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            FKGrpcService::VerifyCodeResponseBody response;
            auto status = stub->GetVerifyCode(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto encryptPassword(const FKGrpcService::EncryptPasswordRequestBody& request) {
        return FKGrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            FKGrpcService::EncryptPasswordResponseBody response;
            auto status = stub->EncryptPassword(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto authenticatePwdReset(const FKGrpcService::AuthenticatePwdResetRequestBody& request) {
        return FKGrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            FKGrpcService::AuthenticatePwdResetResponseBody response;
            auto status = stub->AuthenticatePwdReset(&context, request, &response);
            return std::pair{ response,status };
            });
    }
};

#endif // !FK_PASSWORD_GRPC_CLIENT_H_
