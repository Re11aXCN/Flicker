#include "FKLogicSystem.h"

#include <print>
#include <random>
#include <string>
#include <vector>
#include <json/json.h>

#include "FKUtils.h"
#include "Asio/FKHttpConnection.h"
#include "Grpc/VerifyCode/FKVerifyCodeGrpcClient.h"
#include "Grpc/Password/FKPasswordGrpcClient.h"
#include "Redis/FKRedisConnectionPool.h"
#include "MySQL/Entity/FKUserEntity.h"
#include "MySQL/Service/FKUserService.h"

using namespace FKVerifyCodeGrpc;
using namespace FKPasswordGrpc;
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

	auto getVerifyCodeFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		// 读取请求体
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Body: \n{}", body);

		// 设置响应头
		connection->getResponse().set(boost::beast::http::field::content_type, "text/json; charset=utf-8");
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
		if (!requestRoot.isMember("data") ||
			!requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty()) {

			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::MISSING_FIELDS);
			responseRoot["message"] = "Missing required field: email";
			connection->getResponse().result(boost::beast::http::status::bad_request);
			boost::beast::ostream(connection->getResponse().body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
			return;
		}

		try {
			// 调用gRPC服务
			FKVerifyCodeGrpcClient client(FKGrpcServicePoolManager::getInstance()->getServicePool<gRPC::ServiceType::VERIFY_CODE_SERVICE>());

			VerifyCodeRequestBody grpcRequest;
			grpcRequest.set_request_type(requestRoot["request_type"].asInt());
			grpcRequest.set_email(requestRoot["data"]["email"].asString());
			auto [grpcResponse, grpcStatus]= client.getVerifyCode(grpcRequest);

			// 根据gRPC状态设置HTTP状态
			if (grpcResponse.status_code() != 0) {
				responseRoot["status_code"] = grpcResponse.status_code();
				responseRoot["message"] = grpcResponse.message();
				connection->getResponse().result(boost::beast::http::status::bad_request);
			}
			else {
				if (grpcStatus.ok()) {
					// 创建data对象
					Json::Value dataObj;
					dataObj["request_type"] = grpcResponse.request_type();
					dataObj["email"] = grpcResponse.email();
					dataObj["verify_code"] = grpcResponse.verify_code();
					responseRoot["data"] = dataObj;
					connection->getResponse().result(boost::beast::http::status::ok);
				}
				else {
					responseRoot["status_code"] = grpcStatus.error_code();
					responseRoot["message"] = grpcStatus.error_message();
					connection->getResponse().result(boost::beast::http::status::request_timeout);
				}
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

	auto registerUserFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		// 读取请求体
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Body: \n{}", body);

		// 设置响应头
		connection->getResponse().set(boost::beast::http::field::content_type, "text/json; charset=utf-8");
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
			return false;
		}

		// 验证必需字段
		if (!requestRoot.isMember("data") ||
			!requestRoot["data"].isMember("username") || requestRoot["data"]["username"].empty() ||
			!requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty() ||
			!requestRoot["data"].isMember("hashed_password") || requestRoot["data"]["hashed_password"].empty() ||
			!requestRoot["data"].isMember("verify_code") || requestRoot["data"]["verify_code"].empty()) {
			
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::MISSING_FIELDS);
			responseRoot["message"] = "Lack of necessary registration information";
			connection->getResponse().result(boost::beast::http::status::bad_request);
			boost::beast::ostream(connection->getResponse().body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
			return false;
		}

		// 获取请求数据
		std::string username = requestRoot["data"]["username"].asString();
		std::string email = requestRoot["data"]["email"].asString();
		std::string hashed_password = requestRoot["data"]["hashed_password"].asString();
		std::string verifyCode = requestRoot["data"]["verify_code"].asString();

		// 验证码前缀（与JavaScript端constants.js中定义一致）
		const std::string VERIFICATION_CODE_PREFIX = "verification_code_";

		try {
			bool verificationSuccess = false;

			FKRedisConnectionPool::getInstance()->executeWithConnection([&](sw::redis::Redis* redis) {
				// 构建Redis键名
				std::string redisKey = VERIFICATION_CODE_PREFIX + email;

				// 查询验证码是否存在
				auto optionalValue = redis->get(redisKey);
				if (!optionalValue) {
					// 验证码不存在或已过期
					std::println("验证码已过期或不存在: {}", email);
					responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::VERIFY_CODE_EXPIRED);
					responseRoot["message"] = "The verification code has expired, please get it again";
					connection->getResponse().result(boost::beast::http::status::bad_request);
					return false;
				}

				// 验证码存在，检查是否匹配
				std::string storedCode = *optionalValue;
				if (storedCode != verifyCode) {
					std::println("验证码不匹配: {} vs {}", storedCode, verifyCode);
					responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::VERIFY_CODE_ERROR);
					responseRoot["message"] = "The verification code is incorrect, please re-enter it";
					connection->getResponse().result(boost::beast::http::status::bad_request);
					return false;
				}

				// 验证码匹配，删除Redis中的验证码（防止重用）
				redis->del(redisKey);
				verificationSuccess = true;
				return true;
			});

			// 如果验证码验证失败，直接返回（错误信息已在lambda中设置）
			if (!verificationSuccess) {
				boost::beast::ostream(connection->getResponse().body())
					<< Json::writeString(Json::StreamWriterBuilder(), responseRoot);
				return false;
			}

			// 如果验证码验证成功，调用gRPC服务加密SHA256密码
			FKPasswordGrpcClient client(FKGrpcServicePoolManager::getInstance()->getServicePool<gRPC::ServiceType::PASSWORD_SERVICE>());

			EncryptPasswordRequestBody grpcRequest;
			grpcRequest.set_request_type(requestRoot["request_type"].asInt());
			grpcRequest.set_hashed_password(requestRoot["data"]["hashed_password"].asString());
			
			auto [grpcResponse, grpcStatus] = client.encryptPassword(grpcRequest);
			// 根据gRPC状态设置HTTP状态
			if (grpcResponse.status_code() != 0) {
				responseRoot["status_code"] = grpcResponse.status_code();
				responseRoot["message"] = grpcResponse.message();
				connection->getResponse().result(boost::beast::http::status::bad_request);
			}
			else {
				if (grpcStatus.ok()) {
					auto result = FKUserService::getInstance()->registerUser(username, email, grpcResponse.encrypted_password(), grpcResponse.salt());

					switch (result) {
					case DbOperator::UserRegisterResult::SUCCESS: {
						responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::SUCCESS);
						responseRoot["message"] = "Registration successful, please go to Login";
						Json::Value dataObj;
						dataObj["request_type"] = grpcResponse.request_type();
						dataObj["encrypted_password"] = "******";
						dataObj["salt"] = "******";
						responseRoot["data"] = dataObj;
						connection->getResponse().result(boost::beast::http::status::ok);
						break;
					}
					case DbOperator::UserRegisterResult::USERNAME_EXISTS:
						responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::USER_EXIST);
						responseRoot["message"] = "Username already exists, please choose another one";
						connection->getResponse().result(boost::beast::http::status::bad_request);
						break;
					case DbOperator::UserRegisterResult::EMAIL_EXISTS:
						responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::USER_EXIST);
						responseRoot["message"] = "Email already exists, please use another one or recover your password";
						connection->getResponse().result(boost::beast::http::status::bad_request);
						break;
					default:
						responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::DATABASE_ERROR);
						responseRoot["message"] = "Failed to register user due to database error";
						connection->getResponse().result(boost::beast::http::status::internal_server_error);
						break;
					}
				}
				else {
					responseRoot["status_code"] = grpcStatus.error_code();
					responseRoot["message"] = grpcStatus.error_message();
					connection->getResponse().result(boost::beast::http::status::request_timeout);
				}
			}
		} catch (const std::exception& e) {
			std::println("注册过程发生异常: {}", e.what());
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::NETWORK_ABNORMAL);
			responseRoot["message"] = "Server Internal Error: " + std::string(e.what());
			connection->getResponse().result(boost::beast::http::status::internal_server_error);
		}

		boost::beast::ostream(connection->getResponse().body())
			<< Json::writeString(Json::StreamWriterBuilder(), responseRoot);
		return true;
	};



	auto loginUserFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		// 读取请求体
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Login Body: \n{}", body);

		// 设置响应头
		connection->getResponse().set(boost::beast::http::field::content_type, "text/json; charset=utf-8");
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
			return false;
		}

		// 验证必需字段
		if (!requestRoot.isMember("data") || 
			((!requestRoot["data"].isMember("username") || requestRoot["data"]["username"].empty()) && 
			(!requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty())) ||
			!requestRoot["data"].isMember("hashed_password") || requestRoot["data"]["hashed_password"].empty()) {
			
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::MISSING_FIELDS);
			responseRoot["message"] = "Missing username/email or password";
			connection->getResponse().result(boost::beast::http::status::bad_request);
			boost::beast::ostream(connection->getResponse().body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
			return false;
		}

		// 获取请求数据
		std::string username = requestRoot["data"].isMember("username") ? requestRoot["data"]["username"].asString() : "";
		std::string email = requestRoot["data"].isMember("email") ? requestRoot["data"]["email"].asString() : "";
		std::string hashed_password = requestRoot["data"]["hashed_password"].asString();
		/*bool rememberPassword = requestRoot["data"].isMember("remember_password") ? requestRoot["data"]["remember_password"].asBool() : false;
		bool autoLogin = requestRoot["data"].isMember("auto_login") ? requestRoot["data"]["auto_login"].asBool() : false;*/

		try {
			// 1. 查询用户
			std::optional<FKUserEntity> user;
			if (!username.empty()) {
				user = FKUserService::getInstance()->findUserByUsername(username);
			} else if (!email.empty()) {
				user = FKUserService::getInstance()->findUserByEmail(email);
			}
			
			// 检查用户是否存在
			if (!user.has_value()) {
				std::println("用户不存在: {} / {}", username, email);
				responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::NOT_FIND_USER);
				responseRoot["message"] = "User does not exist {" + username + " / " + email + "}";
				connection->getResponse().result(boost::beast::http::status::bad_request);
				boost::beast::ostream(connection->getResponse().body())
					<< Json::writeString(Json::StreamWriterBuilder(), responseRoot);
				return false;
			}
			
			// 2. 验证密码
			FKPasswordGrpcClient client(FKGrpcServicePoolManager::getInstance()->getServicePool<gRPC::ServiceType::PASSWORD_SERVICE>());

			AuthenticatePasswordRequestBody grpcRequest;
			grpcRequest.set_request_type(requestRoot["request_type"].asInt());
			grpcRequest.set_hashed_password(hashed_password);
			grpcRequest.set_encrypted_password(user->getPassword());

			auto [grpcResponse, grpcStatus] = client.authenticatePassword(grpcRequest);

			if (grpcResponse.status_code() != 0) {
				std::println("密码错误: {}", user->getUsername());
				responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::PASSWORD_ERROR);
				responseRoot["message"] = "Incorrect password";
				connection->getResponse().result(boost::beast::http::status::bad_request);
			}
			else {
				if (grpcStatus.ok()) {
					// 3. 登录成功，更新响应数据


				// TODO: 生成会话令牌并存储在Redis中
				// TODO: 如果选择了记住密码，生成长期令牌

				// 4. 设置成功响应
					responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::SUCCESS);
					responseRoot["message"] = "Login successful";
					Json::Value dataObj;
					dataObj["request_type"] = static_cast<int>(Http::RequestSeviceType::LOGIN_USER);
					responseRoot["data"] = dataObj;
					connection->getResponse().result(boost::beast::http::status::ok);
				}
				else {
					responseRoot["status_code"] = grpcStatus.error_code();
					responseRoot["message"] = grpcStatus.error_message();
					connection->getResponse().result(boost::beast::http::status::request_timeout);
				}
			}
		} catch (const std::exception& e) {
			// 处理异常
			std::println("登录过程发生异常: {}", e.what());
			responseRoot["status_code"] = static_cast<int>(Http::RequestStatusCode::NETWORK_ABNORMAL);
			responseRoot["message"] = "Server Internal Error: " + std::string(e.what());
			connection->getResponse().result(boost::beast::http::status::internal_server_error);
		}

		// 发送响应
		boost::beast::ostream(connection->getResponse().body())
			<< Json::writeString(Json::StreamWriterBuilder(), responseRoot);
		return true;
	};

	auto resetPasswordFunc = [](std::shared_ptr<FKHttpConnection> connection) {

		};

	this->registerCallback("/get_test", Http::RequestType::GET, getTestFunc);
	this->registerCallback("/get_verify_code", Http::RequestType::POST, getVerifyCodeFunc);
	this->registerCallback("/register_user", Http::RequestType::POST, registerUserFunc);
	this->registerCallback("/login_user", Http::RequestType::POST, loginUserFunc);
	this->registerCallback("/reset_password", Http::RequestType::POST, resetPasswordFunc);
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
		response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
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
