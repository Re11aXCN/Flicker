#include "FKVerifyGrpcClinet.h"
#include <print>
SINGLETON_CREATE_SHARED_CPP(FKVerifyGrpcClinet)
FKVerifyGrpcClinet::FKVerifyGrpcClinet()
{
	std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	_pStub = VarifyCodeService::NewStub(channel);
}

VarifyCodeResponseBody FKVerifyGrpcClinet::getVarifyCode(const VarifyCodeRequestBody& requestBody)
{
	ClientContext context;
	VarifyCodeResponseBody response;

	// 设置超时时间（10秒）
	std::chrono::system_clock::time_point deadline =
		std::chrono::system_clock::now() + std::chrono::seconds(10);
	context.set_deadline(deadline);

	// 调用gRPC服务
	Status status = _pStub->GetVarifyCode(&context, requestBody, &response);
	if (!status.ok()) {
		std::println("gRPC call failed: {}", status.error_message());

		// 清空可能的部分响应
		response.Clear();

		// 设置错误状态码
		response.set_status_code(static_cast<int32_t>(Http::RequestStatusCode::GRPC_CALL_FAILED));
		response.set_message("gRPC service error: " + status.error_message());

		// 设置默认值防止未定义行为
		response.set_request_type(0);
		response.set_email("");
		response.set_varify_code("");
	}

	return response;
}

