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
#include "Grpc/FKVerifyGrpcClient.h"
#include "FKUtils.h"

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
			VarifyCodeResponseBody grpcResponse = FKVerifyGrpcClient::getInstance()->getVarifyCode(grpcRequest);

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
	try {
		// 记录请求信息
		std::println("处理请求: {} {}", 
			(requestType == Http::RequestType::GET ? "GET" : "POST"), 
			url);
		
		// 根据请求类型查找对应的回调函数
		if (requestType == Http::RequestType::GET) {
			// 检查GET回调是否存在
			auto it = _pGetRequestCallBacks.find(url);
			if (it == _pGetRequestCallBacks.end()) {
				std::println("未找到GET处理函数: {}", url);
				return false;
			}
			
			// 执行回调函数
			std::println("执行GET处理函数: {}", url);
			it->second(connection);
		}
		else if (requestType == Http::RequestType::POST) {
			// 检查POST回调是否存在
			auto it = _pPostRequestCallBacks.find(url);
			if (it == _pPostRequestCallBacks.end()) {
				std::println("未找到POST处理函数: {}", url);
				return false;
			}
			
			// 执行回调函数
			std::println("执行POST处理函数: {}", url);
			it->second(connection);
		}
		else {
			// 不支持的请求类型
			std::println("不支持的请求类型");
			return false;
		}
		
		return true;
	}
	catch (const std::exception& ex) {
		// 处理回调执行过程中的异常
		std::println("回调执行异常: {}", ex.what());
		
		// 设置500错误响应
		auto& response = connection->getResponse();
		response.result(boost::beast::http::status::internal_server_error);
		response.set(boost::beast::http::field::content_type, "application/json");
		response.set(boost::beast::http::field::server, "GateServer");
		response.set(boost::beast::http::field::date, FKUtils::get_http_date());
		
		// 构建错误JSON响应
		boost::beast::ostream(response.body()) << 
			"{\"code\":500,\"message\":\"服务器内部错误\",\"error\":\"" << 
			ex.what() << "\"}";
		
		return true; // 返回true表示已处理请求
	}
	catch (...) {
		// 处理未知异常
		std::println("回调执行发生未知异常");
		
		// 设置500错误响应
		auto& response = connection->getResponse();
		response.result(boost::beast::http::status::internal_server_error);
		response.set(boost::beast::http::field::content_type, "application/json");
		response.set(boost::beast::http::field::server, "GateServer");
		response.set(boost::beast::http::field::date, FKUtils::get_http_date());
		
		// 构建错误JSON响应
		boost::beast::ostream(response.body()) << 
			"{\"code\":500,\"message\":\"服务器内部错误\",\"error\":\"未知异常\"}";
		
		return true; // 返回true表示已处理请求
	}
}

void FKLogicSystem::registerCallback(const std::string& url, Http::RequestType requestType, MessageHandler handler)
{
	// 检查参数有效性
	if (url.empty()) {
		std::println("错误: 尝试注册空URL的回调函数");
		return;
	}
	
	if (!handler) {
		std::println("错误: 尝试注册空回调函数, URL: {}", url);
		return;
	}
	
	// 根据请求类型注册回调
	if (requestType == Http::RequestType::GET) {
		// 检查是否已存在
		if (_pGetRequestCallBacks.find(url) != _pGetRequestCallBacks.end()) {
			std::println("警告: 覆盖已存在的GET回调函数, URL: {}", url);
		}
		
		// 注册回调
		_pGetRequestCallBacks[url] = handler;
		std::println("成功注册GET回调函数: {}", url);
	}
	else if (requestType == Http::RequestType::POST) {
		// 检查是否已存在
		if (_pPostRequestCallBacks.find(url) != _pPostRequestCallBacks.end()) {
			std::println("警告: 覆盖已存在的POST回调函数, URL: {}", url);
		}
		
		// 注册回调
		_pPostRequestCallBacks[url] = handler;
		std::println("成功注册POST回调函数: {}", url);
	}
	else {
		std::println("错误: 尝试注册不支持的请求类型回调函数, URL: {}", url);
	}
}
