#ifndef GRPC_SERVICE_CLIENT_H_
#define GRPC_SERVICE_CLIENT_H_

#include "Common/grpc/grpc_service_pool_manager.h"

template<flicker::grpc::service T>
class GrpcServiceClient {
public:
    GrpcServiceClient() = default;
    ~GrpcServiceClient() = default;
    GrpcServiceClient(const GrpcServiceClient&) = delete;
    GrpcServiceClient& operator=(const GrpcServiceClient&) = delete;
    GrpcServiceClient(GrpcServiceClient&&) = delete;
    GrpcServiceClient& operator=(GrpcServiceClient&&) = delete;

    auto getVerifyCode(const GrpcService::VerifyCodeRequestBody& request) {
        return GrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            GrpcService::VerifyCodeResponseBody response;
            auto status = stub->GetVerifyCode(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto encryptPassword(const GrpcService::EncryptPasswordRequestBody& request) {
        return GrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            GrpcService::EncryptPasswordResponseBody response;
            auto status = stub->EncryptPassword(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto authenticatePwdReset(const GrpcService::AuthenticatePwdResetRequestBody& request) {
        return GrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            GrpcService::AuthenticatePwdResetResponseBody response;
            auto status = stub->AuthenticatePwdReset(&context, request, &response);
            return std::pair{ response,status };
            });
    }
    auto getChatServerAddress(const GrpcService::GetChatServerAddressRequestBody& request) {
        return GrpcServicePoolManager::getInstance()->getServicePool<T>().executeWithConnection([&](auto* stub) {
            grpc::ClientContext context;
            GrpcService::GetChatServerAddressResponseBody response;
            auto status = stub->GetChatServerAddress(&context, request, &response);
            return std::pair{ response,status };
            });
    }
};

#endif // !GRPC_SERVICE_CLIENT_H_
