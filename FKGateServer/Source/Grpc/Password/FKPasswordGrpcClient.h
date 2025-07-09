#ifndef FK_PASSWORD_GRPC_CLIENT_H_
#define FK_PASSWORD_GRPC_CLIENT_H_

#include "../FKGrpcServicePoolManager.h"
class FKPasswordGrpcClient {
public:
	struct EncryptPasswordGrpcCallResult {
		FKPasswordGrpc::EncryptPasswordResponseBody response;
		grpc::Status status;
	};
	struct AuthenticatePasswordGrpcCallResult {
		FKPasswordGrpc::AuthenticatePasswordResponseBody response;
		grpc::Status status;
	};
	explicit FKPasswordGrpcClient(FKGrpcConnectionPool<FKPasswordGrpc::PasswordService>& pool)
		: _pool(pool) {
	}
	~FKPasswordGrpcClient() = default;
	EncryptPasswordGrpcCallResult encryptPassword(const FKPasswordGrpc::EncryptPasswordRequestBody& request) {
		return _pool.executeWithConnection([&](auto* stub) {
			grpc::ClientContext context;
			FKPasswordGrpc::EncryptPasswordResponseBody response;
			auto status = stub->EncryptPassword(&context, request, &response);
			return EncryptPasswordGrpcCallResult{ response,status };
			});
	}
	AuthenticatePasswordGrpcCallResult authenticatePassword(const FKPasswordGrpc::AuthenticatePasswordRequestBody& request) {
		return _pool.executeWithConnection([&](auto* stub) {
			grpc::ClientContext context;
			FKPasswordGrpc::AuthenticatePasswordResponseBody response;
			auto status = stub->AuthenticatePassword(&context, request, &response);
			return AuthenticatePasswordGrpcCallResult{ response,status };
			});
	}
private:
	FKGrpcConnectionPool<FKPasswordGrpc::PasswordService>& _pool;
};

#endif // !FK_PASSWORD_GRPC_CLIENT_H_
