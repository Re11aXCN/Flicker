#include "FKLogicSystem.h"

#include <random>
#include <string>
#include <vector>
#include <json/json.h>

#include "FKLogger.h"
#include "FKUtils.h"
#include "Asio/FKHttpConnection.h"
#include "Redis/FKRedisConnectionPool.h"
#include "MySQL/Entity/FKUserEntity.h"
#include "MySQL/Mapper/FKUserMapper.h"
#include "Grpc/FKGrpcServiceClient.hpp"

using namespace FKGrpcService;
SINGLETON_CREATE_SHARED_CPP(FKLogicSystem)

FKLogicSystem::FKLogicSystem()
{
    auto getVerifyCodeFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        // 读取请求体
        std::string body = flicker::buffers_to_string(connection->getRequest().body().data());
        FK_SERVER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        // 设置响应头
        httpResponse.set(flicker::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(flicker::http::field::server, "GateServer");
        httpResponse.set(flicker::http::field::date, FKUtils::get_gmtime());
        Json::Value responseRoot;
        // 解析JSON
        Json::Value requestRoot;
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

        if (!isValidJson) {
            FK_SERVER_ERROR(std::format("Invalid JSON format: {}", errors));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(flicker::http::status::bad_request);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        // 验证必需字段
        if (!requestRoot.isMember("request_service_type") || requestRoot["request_service_type"] != static_cast<int>(flicker::http::service::VerifyCode) ||
            !requestRoot.isMember("data") ||
            !requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty() ||
            !requestRoot["data"].isMember("verify_type") || requestRoot["data"]["verify_type"].empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
            responseRoot["message"] = "The user does not have access permissions!";
            httpResponse.result(flicker::http::status::unauthorized);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }
        std::string email = requestRoot["data"]["email"].asString();
        flicker::http::service serviceType = static_cast<flicker::http::service>(requestRoot["data"]["verify_type"].asInt());
        try {
            FKUserMapper mapper;
            std::optional<FKUserEntity> user = mapper.findByEmail(email);
            if (serviceType == flicker::http::service::Register && user.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::conflict);
                responseRoot["message"] = FKUtils::concat("The user '", email, "'already exist! Please choose another one!");
                httpResponse.result(flicker::http::status::conflict);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }
            else if (serviceType == flicker::http::service::ResetPassword &&!user.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
                responseRoot["message"] = FKUtils::concat("The user '", email, "' does not exist!");
                httpResponse.result(flicker::http::status::unauthorized);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }
            else {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
                responseRoot["message"] = "Wrong request, refusal to respond to the service!";
                httpResponse.result(flicker::http::status::bad_request);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }

            // 调用grpc服务
            FKGrpcServiceClient<flicker::grpc::service::VerifyCode> client;

            VerifyCodeRequestBody grpcRequest;
            grpcRequest.set_rpc_request_type(static_cast<int32_t>(flicker::http::service::VerifyCode));
            grpcRequest.set_email(email);
            auto [grpcResponse, grpcStatus]= client.getVerifyCode(grpcRequest);

            // 根据grpc状态设置HTTP状态
            if (grpcResponse.rpc_response_code() != 0) { // 自定义的状态码
                FK_SERVER_ERROR(std::format("grpc 内部错误: {}", grpcResponse.message()));
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(flicker::http::status::service_unavailable);
            }
            else {
                if (grpcStatus.ok()) { // grpc内部的 status
                    // 创建data对象
                    Json::Value dataObj;
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::ok);
                    responseRoot["message"] = "The verification code is successfully sent to the email address and is valid within five minutes!";
                    responseRoot["data"] = dataObj;

                    dataObj["request_service_type"] = requestRoot["request_service_type"];
                    dataObj["verify_type"] = requestRoot["data"]["verify_type"];
                    dataObj["verify_code"] = grpcResponse.verify_code();
                    httpResponse.result(flicker::http::status::ok);
                }
                else {
                    FK_SERVER_ERROR(std::format("grpc 调用失败: {}", FKUtils::concat("grpc error! code: ", magic_enum::enum_name(grpcStatus.error_code()), ", message: ", grpcStatus.error_message())));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::request_timeout);
                    responseRoot["message"] = "The request timed out! Please try again later!";
                    httpResponse.result(flicker::http::status::request_timeout);
                }
            }
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("获取验证码服务回调执行异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(flicker::http::status::internal_server_error);
        }

        // 发送响应
        flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
        };

    auto registerUserFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = flicker::buffers_to_string(connection->getRequest().body().data());
        FK_SERVER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(flicker::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(flicker::http::field::server, "GateServer");
        httpResponse.set(flicker::http::field::date, FKUtils::get_gmtime());

        Json::Value responseRoot, requestRoot;
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

        if (!isValidJson) {
            FK_SERVER_ERROR(std::format("Invalid JSON format: {}", errors));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(flicker::http::status::bad_request);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        if (!requestRoot.isMember("request_service_type") || requestRoot["request_service_type"] != static_cast<int>(flicker::http::service::Register) ||
            !requestRoot.isMember("data") ||
            !requestRoot["data"].isMember("username") || requestRoot["data"]["username"].empty() ||
            !requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty() ||
            !requestRoot["data"].isMember("hashed_password") || requestRoot["data"]["hashed_password"].empty() ||
            !requestRoot["data"].isMember("verify_code") || requestRoot["data"]["verify_code"].empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary registration information!";
            httpResponse.result(flicker::http::status::unauthorized);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        std::string username = requestRoot["data"]["username"].asString();
        std::string email = requestRoot["data"]["email"].asString();
        std::string hashedPassword = requestRoot["data"]["hashed_password"].asString();
        std::string verifyCode = requestRoot["data"]["verify_code"].asString();

        // 验证码前缀（与JavaScript端constants.js中定义一致）
        const std::string VERIFICATION_CODE_PREFIX = "verification_code_";

        try {
            // 1. 查询用户是否存在，关于邮箱查询已经在获取验证码时检查过了
            FKUserMapper mapper;
            std::optional<FKUserEntity> user = mapper.findByUsername(username);
            if (user.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::conflict);
                responseRoot["message"] = FKUtils::concat("The user '", username, "' already exist! Please choose another one!");
                httpResponse.result(flicker::http::status::conflict);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }

            // 2. Redis 验证验证码
            bool verificationSuccess = false;
            FKRedisConnectionPool::getInstance()->executeWithConnection([&](sw::redis::Redis* redis) {
                // 构建Redis键名
                std::string redisKey = VERIFICATION_CODE_PREFIX + email;

                // 查询验证码是否存在
                auto optionalValue = redis->get(redisKey);
                if (!optionalValue) {
                    FK_SERVER_ERROR(std::format("验证码已过期或不存在: {}", email));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::forbidden);
                    responseRoot["message"] = "The verification code has expired, please get it again";
                    httpResponse.result(flicker::http::status::forbidden);
                    return false;
                }

                // 验证码存在，检查是否匹配
                std::string storedCode = *optionalValue;
                if (storedCode != verifyCode) {
                    FK_SERVER_ERROR(std::format("验证码不匹配: {} vs {}", storedCode, verifyCode));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
                    responseRoot["message"] = "The verification code is incorrect, please re-enter it";
                    httpResponse.result(flicker::http::status::unauthorized);
                    return false;
                }

                // 验证码匹配，删除Redis中的验证码（防止重用）
                redis->del(redisKey);
                verificationSuccess = true;
                return true;
            });

            // 如果验证码验证失败，直接返回（错误信息已在lambda中设置）
            if (!verificationSuccess) {
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }

            // 3. 调用grpc服务 bcrypt加密SHA256密码
            FKGrpcServiceClient<flicker::grpc::service::EncryptPassword> client;
            EncryptPasswordRequestBody grpcRequest;
            grpcRequest.set_rpc_request_type(static_cast<int32_t>(flicker::grpc::service::EncryptPassword));
            grpcRequest.set_hashed_password(hashedPassword);
            
            auto [grpcResponse, grpcStatus] = client.encryptPassword(grpcRequest);
            if (grpcResponse.rpc_response_code() != 0) {
                FK_SERVER_ERROR(std::format("grpc 内部错误: {}", grpcResponse.message()));
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(flicker::http::status::service_unavailable);
            }
            else {
                if (grpcStatus.ok()) {
                    FKUserMapper mapper;
                    // 插入用户
                    auto result = mapper.insert(FKUserEntity{ username, email, grpcResponse.encrypted_password() });
                    if (result == DbOperator::Status::Success) [[likely]] {
                        Json::Value dataObj;
                        responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::ok);
                        responseRoot["message"] = "Register successful!";
                        responseRoot["data"] = dataObj;
                        dataObj["request_service_type"] = requestRoot["request_service_type"];
                        httpResponse.result(flicker::http::status::ok);
                    }
                    else [[unlikely]] {
                        responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                        responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                        httpResponse.result(flicker::http::status::service_unavailable);
                    }
                }
                else {
                    FK_SERVER_ERROR(std::format("grpc 调用失败: {}", FKUtils::concat("grpc error! code: ", magic_enum::enum_name(grpcStatus.error_code()), ", message: ", grpcStatus.error_message())));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::request_timeout);
                    responseRoot["message"] = "The request timed out! Please try again later!";
                    httpResponse.result(flicker::http::status::request_timeout);
                }
            }
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("注册用户服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(flicker::http::status::internal_server_error);
        }

        flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
    };

    auto loginUserFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = flicker::buffers_to_string(connection->getRequest().body().data());
        FK_SERVER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(flicker::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(flicker::http::field::server, "GateServer");
        httpResponse.set(flicker::http::field::date, FKUtils::get_gmtime());

        Json::Value responseRoot, requestRoot;
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

        if (!isValidJson) {
            FK_SERVER_ERROR(std::format("Invalid JSON format: {}", errors));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(flicker::http::status::bad_request);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        if (!requestRoot.isMember("request_service_type") || requestRoot["request_service_type"] != static_cast<int>(flicker::http::service::Login) ||
            !requestRoot.isMember("data") || 
            !requestRoot["data"].isMember("username") || requestRoot["data"]["username"].empty() || 
            !requestRoot["data"].isMember("hashed_password") || requestRoot["data"]["hashed_password"].empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary login information!";
            httpResponse.result(flicker::http::status::unauthorized);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        std::string username = requestRoot["data"].isMember("username") ? requestRoot["data"]["username"].asString() : "";
        std::string hashedPassword = requestRoot["data"]["hashed_password"].asString();

        try {
            // 1. 查询用户
            FKUserMapper mapper;
            std::optional<std::string> bcryptPassword = username.contains("@")
                ? mapper.findPasswordByEmail(username)
                : mapper.findPasswordByUsername(username);
            
            if (!bcryptPassword.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
                responseRoot["message"] = FKUtils::concat("The user '", username, "' does not exist!");
                httpResponse.result(flicker::http::status::unauthorized);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }
            
            // 2. 验证密码
            FKGrpcServiceClient<flicker::grpc::service::AuthenticatePwdReset> client;

            AuthenticatePwdResetRequestBody grpcRequest;
            grpcRequest.set_rpc_request_type(static_cast<int32_t>(flicker::grpc::service::AuthenticatePwdReset));
            grpcRequest.set_hashed_password(hashedPassword);
            grpcRequest.set_encrypted_password(bcryptPassword.value());

            auto [grpcResponse, grpcStatus] = client.authenticatePwdReset(grpcRequest);

            if (grpcResponse.rpc_response_code() != 0) {
                FK_SERVER_ERROR(std::format("grpc 内部错误: {}", grpcResponse.message()));
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(flicker::http::status::service_unavailable);
            }
            else {
                if (grpcStatus.ok()) {
                    // 3. 登录成功，更新响应数据


                    // TODO: 生成会话令牌并存储在Redis中
                    // TODO: 如果选择了记住密码，生成长期令牌

                    // 4. 设置成功响应
                    Json::Value dataObj;
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::ok);
                    responseRoot["message"] = "Login successful!";
                    responseRoot["data"] = dataObj;
                    dataObj["request_service_type"] = requestRoot["request_service_type"];
                    httpResponse.result(flicker::http::status::ok);
                }
                else {
                    FK_SERVER_ERROR(std::format("grpc 调用失败: {}", FKUtils::concat("grpc error! code: ", magic_enum::enum_name(grpcStatus.error_code()), ", message: ", grpcStatus.error_message())));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::request_timeout);
                    responseRoot["message"] = "The request timed out! Please try again later!";
                    httpResponse.result(flicker::http::status::request_timeout);
                }
            }
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("登录服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(flicker::http::status::internal_server_error);
        }

        // 发送响应
        flicker::ostream(httpResponse.body())
            << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
    };

    auto authenticateUserFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = flicker::buffers_to_string(connection->getRequest().body().data());
        FK_SERVER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(flicker::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(flicker::http::field::server, "GateServer");
        httpResponse.set(flicker::http::field::date, FKUtils::get_gmtime());

        Json::Value responseRoot, requestRoot;
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

        if (!isValidJson) {
            FK_SERVER_ERROR(std::format("Invalid JSON format: {}", errors));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(flicker::http::status::bad_request);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        if (!requestRoot.isMember("request_service_type") || requestRoot["request_service_type"] != static_cast<int>(flicker::http::service::ResetPassword) ||
            !requestRoot.isMember("data") ||
            !requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty() ||
            !requestRoot["data"].isMember("verify_code") || requestRoot["data"]["verify_code"].empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary authenticate password reset information!";
            httpResponse.result(flicker::http::status::unauthorized);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        std::string email = requestRoot["data"]["email"].asString();
        std::string verifyCode = requestRoot["data"]["verify_code"].asString();
        const std::string VERIFICATION_CODE_PREFIX = "verification_code_";

        try {
            FKUserMapper mapper;
            std::optional<FKUserEntity> user = mapper.findByEmail(email);
            if (!user.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::conflict);
                responseRoot["message"] = FKUtils::concat("The user '", email, "' does not exist, please register first!");
                httpResponse.result(flicker::http::status::conflict);
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }

            bool verificationSuccess = false;
            FKRedisConnectionPool::getInstance()->executeWithConnection([&](sw::redis::Redis* redis) {
                std::string redisKey = VERIFICATION_CODE_PREFIX + email;

                auto optionalValue = redis->get(redisKey);
                if (!optionalValue) {
                    FK_SERVER_ERROR(std::format("验证码已过期或不存在: {}", email));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::forbidden);
                    responseRoot["message"] = "The verification code has expired, please get it again";
                    httpResponse.result(flicker::http::status::forbidden);
                    return false;
                }

                std::string storedCode = *optionalValue;
                if (storedCode != verifyCode) {
                    FK_SERVER_ERROR(std::format("验证码不匹配: {} vs {}", storedCode, verifyCode));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
                    responseRoot["message"] = "The verification code is incorrect, please re-enter it";
                    httpResponse.result(flicker::http::status::unauthorized);
                    return false;
                }

                redis->del(redisKey);
                verificationSuccess = true;
                return true;
                });

            // 如果验证码验证失败，直接返回（错误信息已在lambda中设置）
            if (!verificationSuccess) {
                flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
                return;
            }

            Json::Value dataObj;
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::ok);
            responseRoot["message"] = "Authentication successful!";
            responseRoot["data"] = dataObj;
            dataObj["request_service_type"] = requestRoot["request_service_type"];
            httpResponse.result(flicker::http::status::ok);
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("注册用户服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(flicker::http::status::internal_server_error);
        }

        flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
        };

    auto resetPasswordFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = flicker::buffers_to_string(connection->getRequest().body().data());
        FK_SERVER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(flicker::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(flicker::http::field::server, "GateServer");
        httpResponse.set(flicker::http::field::date, FKUtils::get_gmtime());
        
        Json::Value responseRoot, requestRoot;
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        bool isValidJson = reader->parse(body.c_str(), body.c_str() + body.size(), &requestRoot, &errors);

        if (!isValidJson) {
            FK_SERVER_ERROR(std::format("Invalid JSON format: {}", errors));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(flicker::http::status::bad_request);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        if (!requestRoot.isMember("request_service_type") || requestRoot["request_service_type"] != static_cast<int>(flicker::http::service::ResetPassword) ||
            !requestRoot.isMember("data") ||
            !requestRoot["data"].isMember("email") || requestRoot["data"]["email"].empty() ||
            !requestRoot["data"].isMember("hashed_password") || requestRoot["data"]["hashed_password"].empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary reset password information!";
            httpResponse.result(flicker::http::status::unauthorized);
            flicker::ostream(httpResponse.body()) << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
            return;
        }

        std::string email = requestRoot["data"]["email"].asString();
        std::string hashedPassword = requestRoot["data"]["hashed_password"].asString();

        try {
            FKGrpcServiceClient<flicker::grpc::service::EncryptPassword> client;
            EncryptPasswordRequestBody grpcRequest;
            grpcRequest.set_rpc_request_type(static_cast<int32_t>(flicker::grpc::service::EncryptPassword));
            grpcRequest.set_hashed_password(hashedPassword);

            auto [grpcResponse, grpcStatus] = client.encryptPassword(grpcRequest);

            if (grpcResponse.rpc_response_code() != 0) {
                FK_SERVER_ERROR(std::format("grpc 内部错误: {}", grpcResponse.message()));
                responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(flicker::http::status::service_unavailable);
            }
            else {
                if (grpcStatus.ok()) {
                    FKUserMapper mapper;
                    auto result = mapper.updatePasswordByEmail(email, grpcResponse.encrypted_password());
                    if (result == DbOperator::Status::Success) [[likely]] {
                        Json::Value dataObj;
                        responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::ok);
                        responseRoot["message"] = "Reset password successful!";
                        responseRoot["data"] = dataObj;
                        dataObj["request_type"] = static_cast<int>(flicker::http::service::ResetPassword);
                        httpResponse.result(flicker::http::status::ok);
                    }
                    else [[unlikely]] {
                        responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::service_unavailable);
                        responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                        httpResponse.result(flicker::http::status::service_unavailable);
                    }
                }
                else {
                    FK_SERVER_ERROR(std::format("grpc 调用失败: {}", FKUtils::concat("grpc error! code: ", magic_enum::enum_name(grpcStatus.error_code()), ", message: ", grpcStatus.error_message())));
                    responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::request_timeout);
                    responseRoot["message"] = "The request timed out! Please try again later!";
                    httpResponse.result(flicker::http::status::request_timeout);
                }
            }
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("重置密码服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(flicker::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(flicker::http::status::internal_server_error);
        }

        flicker::ostream(httpResponse.body())
            << Json::writeString(Json::StreamWriterBuilder(), responseRoot);
        };

    this->registerCallback("/get_verify_code", flicker::http::verb::post, getVerifyCodeFunc);
    this->registerCallback("/login_user", flicker::http::verb::post, loginUserFunc);
    this->registerCallback("/register_user", flicker::http::verb::post, registerUserFunc);
    this->registerCallback("/authenticate_user", flicker::http::verb::post, authenticateUserFunc);
    this->registerCallback("/reset_password", flicker::http::verb::post, resetPasswordFunc);

}

bool FKLogicSystem::callBack(const std::string& url, flicker::http::verb requestType, std::shared_ptr<FKHttpConnection> connection)
{
    if (requestType == flicker::http::verb::get) {
        auto it = _pGetRequestCallBacks.find(url);
        if (it == _pGetRequestCallBacks.end()) {
            FK_SERVER_ERROR(std::format("服务器未注册当前服务的GET处理函数: {}", url));
            return false;
        }

        FK_SERVER_INFO(std::format("执行GET处理函数: {}", url));
        it->second(connection);
    }
    else if (requestType == flicker::http::verb::post) {
        auto it = _pPostRequestCallBacks.find(url);
        if (it == _pPostRequestCallBacks.end()) {
            FK_SERVER_ERROR(std::format("服务器未注册当前服务的POST处理函数: {}", url));
            return false;
        }

        FK_SERVER_INFO(std::format("执行POST处理函数: {}", url));
        it->second(connection);
    }
    else {
        return false;
    }

    return true;
}

void FKLogicSystem::registerCallback(const std::string& url, flicker::http::verb requestType, MessageHandler handler)
{
    // 检查参数有效性
    if (url.empty()) {
        FK_SERVER_ERROR("尝试注册空URL的回调函数");
        return;
    }
    
    if (!handler) {
        FK_SERVER_ERROR(std::format("尝试注册空回调函数, URL: {}", url));
        return;
    }
    
    if (requestType == flicker::http::verb::get) {
        if (_pGetRequestCallBacks.find(url) != _pGetRequestCallBacks.end()) {
            FK_SERVER_WARN(std::format("覆盖已存在的GET回调函数, URL: {}", url));
        }
        
        _pGetRequestCallBacks[url] = handler;
        FK_SERVER_INFO(std::format("成功注册GET回调函数: {}", url));
    }
    else if (requestType == flicker::http::verb::post) {
        if (_pPostRequestCallBacks.find(url) != _pPostRequestCallBacks.end()) {
            FK_SERVER_WARN(std::format("覆盖已存在的POST回调函数, URL: {}", url));
        }
        
        _pPostRequestCallBacks[url] = handler;
        FK_SERVER_INFO(std::format("成功注册POST回调函数: {}", url));
    }
    else {
        
    }
}
