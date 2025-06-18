#include "FKVerifyGrpcClinet.h"

SINGLETON_CREATE_SHARED_CPP(FKVerifyGrpcClinet)
FKVerifyGrpcClinet::FKVerifyGrpcClinet()
{
	std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	_pStub = VarifyCodeService::NewStub(channel);
}

VarifyCodeResponseBody FKVerifyGrpcClinet::GetVarifyCode(std::string email)
{
	ClientContext context;
	VarifyCodeResponseBody response;
	VarifyCodeRequestBody request;
	request.set_email(email);

	Status status = _pStub->GetVarifyCode(&context, request, &response);

	if (!status.ok()) {
		response.set_error(Http::RequestErrorCode::RPC_ERROR);
	}

	return response;
}

