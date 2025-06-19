#ifndef FK_VERIFY_GRPC_CLINET_H_
#define FK_VERIFY_GRPC_CLINET_H_
#include <grpcpp/grpcpp.h>
#include "FKVerifyGrpc.grpc.pb.h"
#include "FKMarco.h"
#include "FKDef.h"
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using FKVerifyGrpc::VarifyCodeService;
using FKVerifyGrpc::VarifyCodeRequestBody;
using FKVerifyGrpc::VarifyCodeResponseBody;

class FKVerifyGrpcClinet
{
	SINGLETON_CREATE_SHARED_H(FKVerifyGrpcClinet)
public:
	VarifyCodeResponseBody getVarifyCode(const VarifyCodeRequestBody& requestBody);

private:
	FKVerifyGrpcClinet();
	~FKVerifyGrpcClinet() = default;
	std::unique_ptr<VarifyCodeService::Stub> _pStub;
};
#endif


