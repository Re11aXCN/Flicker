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
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Body: {}", body);
		
		connection->getResponse().set(boost::beast::http::field::content_type, "text/json");
		
		Json::Value root;
		Json::Reader reader;
		bool parse_success = reader.parse(body, root);
		if (!parse_success) [[unlikely]] {
			std::println("Failed to parse JSON data!");
			root["state"] = Http::RequestErrorCode::PARSER_JSON_ERROR;
			boost::beast::ostream(connection->getResponse().body()) << root.toStyledString();
			return;
		}
		
		root["state"] = Http::RequestErrorCode::SUCCESS;
		root["email"] = root["email"].asString();
		boost::beast::ostream(connection->getResponse().body()) << root.toStyledString();
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
