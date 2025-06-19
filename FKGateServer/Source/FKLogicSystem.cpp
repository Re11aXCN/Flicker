/*************************************************************************************
 *
 * @ Filename	 : FKLogicSystem.cpp
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#include "FKLogicSystem.h"

#include <print>

#include <json/json.h>

#include "FKHttpConnection.h"
#include "Grpc/FKVerifyGrpcClinet.h"
SINGLETON_CREATE_SHARED_CPP(FKLogicSystem)

FKLogicSystem::FKLogicSystem()
{
	auto getTestFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		Json::Value root;
		for (const auto& [key, value] : connection->getQueryParams()) {
			root[key] = value; 
		}

		// 序列化JSON为字符串
		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = ""; // 紧凑格式（无缩进）
		std::string jsonOutput = Json::writeString(writerBuilder, root);

		// 输出JSON到响应体
		auto& responseBody = connection->getResponse().body();
		boost::beast::ostream(responseBody) << jsonOutput;
		};

	auto getVarifyCodeFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		// 读取请求体
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Body: \n{}", body);

		// 设置响应头
		connection->getResponse().set(boost::beast::http::field::content_type, "text/json");
		Json::Value responseRoot;
		// 解析JSON
		Json::Value requestRoot;
		Json::CharReaderBuilder readerBuilder;
		std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
		std::string errors;
		bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

		if (!isValidJson) {
			std::println("Invalid JSON format: {}", errors);
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::INVALID_JSON);
			responseRoot["message"] = "Invalid JSON format: " + errors;
			connection->getResponse().result(boost::beast::http::status::bad_request);
			boost::beast::ostream(connection->getResponse().body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
			return;
		}

		// 验证必需字段
		if (!requestRoot.isMember("email") || requestRoot["email"].empty()) {
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::MISSING_FIELDS);
			responseRoot["message"] = "Missing required field: email";
			connection->getResponse().result(boost::beast::http::status::bad_request);
			boost::beast::ostream(connection->getResponse().body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
			return;
		}

		// 构建gRPC请求
		VarifyCodeRequestBody grpcRequest;
		if (requestRoot.isMember("request_type")) {
			grpcRequest.set_request_type(requestRoot["request_type"].asInt());
		}
		else [[unlikely]] {
			// 默认值（根据业务需求设置）
			grpcRequest.set_request_type(0);
		}
		grpcRequest.set_email(requestRoot["email"].asString());

		try {
			// 调用gRPC服务
			VarifyCodeResponseBody grpcResponse = FKVerifyGrpcClinet::getInstance()->getVarifyCode(grpcRequest);

			// 构建JSON响应
			responseRoot["status_code"] = grpcResponse.status_code();
			responseRoot["message"] = grpcResponse.message();
			responseRoot["request_type"] = grpcResponse.request_type();
			responseRoot["email"] = grpcResponse.email();
			responseRoot["varify_code"] = grpcResponse.varify_code();

			// 根据gRPC状态设置HTTP状态
			if (grpcResponse.status_code() >= 400) {
				connection->getResponse().result(boost::beast::http::status::bad_request);
			}
			else {
				connection->getResponse().result(boost::beast::http::status::ok);
			}

		}
		catch (const std::exception& e) {
			// gRPC调用异常处理
			std::println("gRPC call failed: {}", e.what());
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::GRPC_CALL_FAILED);
			responseRoot["message"] = "Service unavailable: " + std::string(e.what());
			connection->getResponse().result(boost::beast::http::status::service_unavailable);
		}

		// 发送响应
		boost::beast::ostream(connection->getResponse().body())
			<< Json::writeString(Json::StreamWriterBuilder(), responseRoot);
		};

	this->registerCallback("/get_test", Http::RequestType::GET, getTestFunc);
	this->registerCallback("/get_varifycode", Http::RequestType::POST, getVarifyCodeFunc);
}

bool FKLogicSystem::callBack(const std::string& url, Http::RequestType requestType, std::shared_ptr<FKHttpConnection> connection)
{
	if (requestType == Http::RequestType::GET) {
		if (_pGetRequestCallBacks.find(url) == _pGetRequestCallBacks.end()) return false;

		_pGetRequestCallBacks[url](connection);
	}
	else {
		if (_pPostRequestCallBacks.find(url) == _pPostRequestCallBacks.end()) return false;

		_pPostRequestCallBacks[url](connection);
	}
	
	return true;
}

void FKLogicSystem::registerCallback(const std::string& url, Http::RequestType requestType, MessageHandler handler)
{
	if (requestType == Http::RequestType::GET) {
		_pGetRequestCallBacks.insert({ url, handler });
	}
	else if (requestType == Http::RequestType::POST) {
		_pPostRequestCallBacks.insert({ url, handler });
	}
}
