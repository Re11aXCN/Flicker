#ifndef FK_VERIFY_CODE_GRPC_CLIENT_H_
#define FK_VERIFY_CODE_GRPC_CLIENT_H_

#include "../FKGrpcServicePoolManager.h"
class FKVerifyCodeGrpcClient {
public:
	struct VerifyCodeGrpcCallResult {
		FKVerifyCodeGrpc::VerifyCodeResponseBody response;
		grpc::Status status;
	};
	explicit FKVerifyCodeGrpcClient(FKGrpcConnectionPool<FKVerifyCodeGrpc::VerifyCodeService>& pool)
		: _pool(pool) {
	}
	~FKVerifyCodeGrpcClient() = default;
	VerifyCodeGrpcCallResult getVerifyCode(const FKVerifyCodeGrpc::VerifyCodeRequestBody& request) {
		return _pool.executeWithConnection([&](auto* stub) {
			grpc::ClientContext context;
			FKVerifyCodeGrpc::VerifyCodeResponseBody response;
			auto status = stub->GetVerifyCode(&context, request, &response);
			return VerifyCodeGrpcCallResult{response, status};
			});
	}

private:
	FKGrpcConnectionPool<FKVerifyCodeGrpc::VerifyCodeService>& _pool;
};

#endif // !FK_VERIFY_CODE_GRPC_CLIENT_H_
