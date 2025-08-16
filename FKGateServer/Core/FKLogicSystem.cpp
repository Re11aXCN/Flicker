#include "FKLogicSystem.h"

#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>

#include "FKHttpConnection.h"

#include "Flicker/Global/universal/utils.h"
#include "Flicker/Global/universal/mysql/connection_pool.h"

#include "Flicker/Global/Grpc/FKGrpcServiceClient.hpp"
#include "Flicker/Global/Mysql/FKUserEntity.h"
#include "Flicker/Global/Mysql/FKUserMapper.h"
#include "Flicker/Global/Redis/FKRedisSingleton.h"
#include "Flicker/Global/Smtp/FKEmailSender.h"

#include "Library/Logger/logger.h"
#include "Library/Bcrypt/bcrypt.h"

using namespace im::service;
using namespace universal;
SINGLETON_CREATE_SHARED_CPP(FKLogicSystem)

FKLogicSystem::FKLogicSystem()
{
    mysql::ConnectionOptions options{
        .username = "root",
        .password = "123456",
        .database = "flicker",
        .read_default_file = utils::string::concat(utils::path::get_env_a<"MYSQL_HOME">().value().string(), utils::path::local_separator(), "data", utils::path::local_separator(), "my.ini"),
        .read_default_group = "mysqld"
    };
    _pFlickerDbPool = mysql::ConnectionPoolManager::getInstance()->get_pool(std::move(options));
    //{
    //    try {
    //        FKUserMapper mapper(_pFlickerDbPool.get());
    //        mapper.createTable();
    //    }
    //    catch (...) {
    //        LOGGER_ERROR("创建 user 表失败!");
    //    }
    //}

    auto getVerifyCodeFunc = [this](std::shared_ptr<FKHttpConnection> connection) {
        // 读取请求体
        std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
        LOGGER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        // 设置响应头
        httpResponse.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(boost::beast::http::field::server, "GateServer");
        httpResponse.set(boost::beast::http::field::date, utils::time::get_gmtime());
        nlohmann::json responseRoot, requestRoot;
        try {
            requestRoot = nlohmann::json::parse(body);
        }
        catch (const nlohmann::json::parse_error& e) {
            LOGGER_ERROR(std::format("JSON解析错误: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        // 验证必需字段
        if (!requestRoot.contains("request_service_type") ||
            requestRoot["request_service_type"] != static_cast<int>(Flicker::Client::Enums::ServiceType::VerifyCode) ||
            !requestRoot.contains("data") ||
            !requestRoot["data"].contains("email") ||
            !requestRoot["data"].contains("verify_type"))
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "The user does not have access permissions!";
            httpResponse.result(boost::beast::http::status::unauthorized);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        std::string email = requestRoot["data"]["email"].get<std::string>();
        Flicker::Client::Enums::ServiceType serviceType = static_cast<Flicker::Client::Enums::ServiceType>(requestRoot["data"]["verify_type"].get<int>());
        bool is_unknow_service = !(serviceType == Flicker::Client::Enums::ServiceType::Register || serviceType == Flicker::Client::Enums::ServiceType::ResetPassword);

        if (email.empty() || is_unknow_service)
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        try {
            FKUserMapper mapper(_pFlickerDbPool.get());
            bool isExists = mapper.isEmailExists(email);
            switch (serviceType) {
            case Flicker::Client::Enums::ServiceType::Register: {
                if (isExists) {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::conflict);
                    responseRoot["message"] = utils::string::concat("The user '", email, "'already exist! Please choose another one!");
                    httpResponse.result(boost::beast::http::status::conflict);
                    boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                    return;
                }
                break;
            }
            case Flicker::Client::Enums::ServiceType::ResetPassword: {
                if (!isExists) {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
                    responseRoot["message"] = utils::string::concat("The user '", email, "' does not exist! Please check the email address!");
                    httpResponse.result(boost::beast::http::status::unauthorized);
                    boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                    return;
                }
                break;
            }
            default: {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
                responseRoot["message"] = "Wrong request, refusal to respond to the service!";
                httpResponse.result(boost::beast::http::status::bad_request);
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            }
            }
            RedisResult result = FKRedisSingleton::generateAndStoreCode(email);
            if (result) {
                if (!FKEmailSender::sendVerificationEmail(email, result.value(), serviceType)) {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                    responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                    httpResponse.result(boost::beast::http::status::service_unavailable);
                }
                else {
                    nlohmann::json dataObj;
                    dataObj["request_service_type"] = requestRoot["request_service_type"];
                    dataObj["verify_type"] = requestRoot["data"]["verify_type"];
                    dataObj["verify_code"] = result.value();
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::ok);
                    responseRoot["message"] = "The verification code is successfully sent to the email address and is valid within five minutes!";
                    responseRoot["data"] = dataObj;

                    httpResponse.result(boost::beast::http::status::ok);
                }
            }
            else {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(boost::beast::http::status::service_unavailable);
            }
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("获取验证码服务回调执行异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(boost::beast::http::status::internal_server_error);
        }

        // 发送响应
        boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
        };

    auto registerUserFunc = [this](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
        LOGGER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(boost::beast::http::field::server, "GateServer");
        httpResponse.set(boost::beast::http::field::date, utils::time::get_gmtime());

        nlohmann::json responseRoot, requestRoot;
        try {
            requestRoot = nlohmann::json::parse(body);
        }
        catch (const nlohmann::json::parse_error& e) {
            LOGGER_ERROR(std::format("JSON解析错误: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        if (!requestRoot.contains("request_service_type") ||
            requestRoot["request_service_type"] != static_cast<int>(Flicker::Client::Enums::ServiceType::Register) ||
            !requestRoot.contains("data") ||
            !requestRoot["data"].contains("username") ||
            !requestRoot["data"].contains("email") ||
            !requestRoot["data"].contains("hashed_password") ||
            !requestRoot["data"].contains("verify_code"))
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary registration information!";
            httpResponse.result(boost::beast::http::status::unauthorized);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        std::string username = requestRoot["data"]["username"].get<std::string>();
        std::string email = requestRoot["data"]["email"].get<std::string>();
        std::string hashedPassword = requestRoot["data"]["hashed_password"].get<std::string>();
        std::string verifyCode = requestRoot["data"]["verify_code"].get<std::string>();

        if (username.empty() || email.empty() || hashedPassword.empty() || verifyCode.empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        try {
            // 1. 查询用户是否存在，关于邮箱查询已经在获取验证码时检查过了
            FKUserMapper mapper(_pFlickerDbPool.get());
            bool isExists = mapper.isUsernameExists(username);
            if (isExists) {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::conflict);
                responseRoot["message"] = utils::string::concat("The user '", username, "' already exist! Please choose another one!");
                httpResponse.result(boost::beast::http::status::conflict);
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            }
            const std::string prefix = "verification_code:";

            // 2. Redis 验证验证码
            RedisResult result = FKRedisSingleton::verifyCode(email, verifyCode);
            if (!result) {
                switch (result.error().code) {
                case RedisErrorCode::ValueExpired: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::forbidden);
                    responseRoot["message"] = "The verification code has expired! Please get it again!";
                    httpResponse.result(boost::beast::http::status::forbidden);
                    break;
                }
                case RedisErrorCode::ValueMismatch: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
                    responseRoot["message"] = "The verification code is incorrect, please re-enter it";
                    httpResponse.result(boost::beast::http::status::unauthorized);
                    break;
                }
                default: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                    responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                    httpResponse.result(boost::beast::http::status::service_unavailable);
                    break;
                }
                }
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            }

            // 3. 插入用户
            auto [status, affectedRows] = mapper.insert(FKUserEntity{ username, email, bcrypt::generateHash(hashedPassword) });
            if (status == mysql::DbOperatorStatus::Success) [[likely]] {
                nlohmann::json dataObj;
                dataObj["request_service_type"] = requestRoot["request_service_type"];
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::ok);
                responseRoot["message"] = "Register successful!";
                responseRoot["data"] = dataObj;
                httpResponse.result(boost::beast::http::status::ok);
            }
            else [[unlikely]] {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(boost::beast::http::status::service_unavailable);
            }
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("注册用户服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(boost::beast::http::status::internal_server_error);
        }

        boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
        };

    auto loginUserFunc = [this](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
        LOGGER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(boost::beast::http::field::server, "GateServer");
        httpResponse.set(boost::beast::http::field::date, utils::time::get_gmtime());

        nlohmann::json responseRoot, requestRoot;
        try {
            requestRoot = nlohmann::json::parse(body);
        }
        catch (const nlohmann::json::parse_error& e) {
            LOGGER_ERROR(std::format("JSON解析错误: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        if (!requestRoot.contains("request_service_type") ||
            requestRoot["request_service_type"] != static_cast<int>(Flicker::Client::Enums::ServiceType::Login) ||
            !requestRoot.contains("data") ||
            !requestRoot["data"].contains("username") ||
            !requestRoot["data"].contains("hashed_password") ||
            !requestRoot["data"].contains("client_device_id"))
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary login information!";
            httpResponse.result(boost::beast::http::status::unauthorized);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        std::string username = requestRoot["data"]["username"].get<std::string>();
        std::string hashedPassword = requestRoot["data"]["hashed_password"].get<std::string>();
        std::string clientDeviceId = requestRoot["data"]["client_device_id"].get<std::string>();

        if (username.empty() || hashedPassword.empty() || clientDeviceId.empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        try {
            // 1. 查询用户
            FKUserMapper mapper(_pFlickerDbPool.get());
            std::optional<FKUserEntity> entity = username.contains("@")
                ? mapper.findByEmail(username)
                : mapper.findByUsername(username);

            if (!entity.has_value()) {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
                responseRoot["message"] = utils::string::concat("The user '", username, "' does not exist! Please register first!");
                httpResponse.result(boost::beast::http::status::unauthorized);
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            }
            // 2. 验证密码
            if (!bcrypt::validatePassword(hashedPassword, entity.value().getPassword())) {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
                responseRoot["message"] = "The password is incorrect, please re-enter it";
                httpResponse.result(boost::beast::http::status::unauthorized);
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            };

            // 3. 向状态服务器请求生成Token和获取聊天服务器信息
            FKGrpcServiceClient<Flicker::Server::Enums::GrpcServiceType::GenerateToken> tokenClient;

            GenerateTokenRequest tokenRequest;
            tokenRequest.set_user_uuid(entity->getUuid());
            tokenRequest.set_client_device_id(clientDeviceId);

            auto [tokenResponse, status] = tokenClient.generateToken(tokenRequest);
            if (status.ok() && tokenResponse.status() == im::service::StatusCode::ok) {
                nlohmann::json data;
                data["user_uuid"] = entity->getUuid();
                data["token"] = tokenResponse.token();
                data["expires_at"] = static_cast<int64_t>(tokenResponse.expires_at());
                data["client_device_id"] = clientDeviceId;
                // 聊天服务器信息
                const auto& chat_server = tokenResponse.chat_server_info();
                data["chat_server_id"] = chat_server.id();
                data["chat_server_host"] = chat_server.host();
                data["chat_server_port"] = chat_server.port();
                data["chat_server_zone"] = chat_server.zone();

                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::ok);
                responseRoot["message"] = "Login successful!";
                responseRoot["data"] = data;
                httpResponse.result(boost::beast::http::status::ok);

                LOGGER_INFO(std::format("用户登录成功: {} -> 聊天服务器: {}:{}", entity->getUuid(), chat_server.host(), chat_server.port()));
            }
            else {
                LOGGER_ERROR(std::format("状态服务器生成Token失败: {}", tokenResponse.error_detail()));
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
                responseRoot["message"] = "Server Internal Error!";
                httpResponse.result(boost::beast::http::status::internal_server_error);
            }
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("登录服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(boost::beast::http::status::internal_server_error);
        }

        boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
        };

    auto authenticateResetPwdFunc = [](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
        LOGGER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(boost::beast::http::field::server, "GateServer");
        httpResponse.set(boost::beast::http::field::date, utils::time::get_gmtime());

        nlohmann::json responseRoot, requestRoot;
        try {
            requestRoot = nlohmann::json::parse(body);
        }
        catch (const nlohmann::json::parse_error& e) {
            LOGGER_ERROR(std::format("JSON解析错误: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        if (!requestRoot.contains("request_service_type") ||
            requestRoot["request_service_type"] != static_cast<int>(Flicker::Client::Enums::ServiceType::AuthenticateResetPwd) ||
            !requestRoot.contains("data") ||
            !requestRoot["data"].contains("email") ||
            !requestRoot["data"].contains("verify_code"))
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary authenticate password reset information!";
            httpResponse.result(boost::beast::http::status::unauthorized);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        std::string email = requestRoot["data"]["email"].get<std::string>();
        std::string verifyCode = requestRoot["data"]["verify_code"].get<std::string>();

        if (email.empty() || verifyCode.empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }
        try {
            RedisResult result = FKRedisSingleton::verifyCode(email, verifyCode);
            if (!result) {
                switch (result.error().code) {
                case RedisErrorCode::ValueExpired: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::forbidden);
                    responseRoot["message"] = "The verification code has expired! Please get it again!";
                    httpResponse.result(boost::beast::http::status::forbidden);
                    break;
                }
                case RedisErrorCode::ValueMismatch: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
                    responseRoot["message"] = "The verification code is incorrect, please re-enter it";
                    httpResponse.result(boost::beast::http::status::unauthorized);
                    break;
                }
                default: {
                    responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                    responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                    httpResponse.result(boost::beast::http::status::service_unavailable);
                    break;
                }
                }
                boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
                return;
            }

            nlohmann::json dataObj;
            dataObj["request_service_type"] = requestRoot["request_service_type"];
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::ok);
            responseRoot["message"] = "Authentication successful!";
            responseRoot["data"] = dataObj;
            httpResponse.result(boost::beast::http::status::ok);
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("注册用户服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(boost::beast::http::status::internal_server_error);
        }

        boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
        };

    auto resetPasswordFunc = [this](std::shared_ptr<FKHttpConnection> connection) {
        std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
        LOGGER_INFO(std::format("收到客户端请求数据: {}", body));
        auto& httpResponse = connection->getResponse();
        httpResponse.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
        httpResponse.set(boost::beast::http::field::server, "GateServer");
        httpResponse.set(boost::beast::http::field::date, utils::time::get_gmtime());

        nlohmann::json responseRoot, requestRoot;
        try {
            requestRoot = nlohmann::json::parse(body);
        }
        catch (const nlohmann::json::parse_error& e) {
            LOGGER_ERROR(std::format("JSON解析错误: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::bad_request);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        if (!requestRoot.contains("request_service_type") ||
            requestRoot["request_service_type"] != static_cast<int>(Flicker::Client::Enums::ServiceType::ResetPassword) ||
            !requestRoot.contains("data") ||
            !requestRoot["data"].contains("email") ||
            !requestRoot["data"].contains("hashed_password"))
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Lack of necessary reset password information!";
            httpResponse.result(boost::beast::http::status::unauthorized);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        std::string email = requestRoot["data"]["email"].get<std::string>();
        std::string hashedPassword = requestRoot["data"]["hashed_password"].get<std::string>();

        if (email.empty() || hashedPassword.empty())
        {
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::unauthorized);
            responseRoot["message"] = "Wrong request, refusal to respond to the service!";
            httpResponse.result(boost::beast::http::status::bad_request);
            boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
            return;
        }

        try {
            FKUserMapper mapper(_pFlickerDbPool.get());
            auto result = mapper.updatePasswordByEmail(email, bcrypt::generateHash(hashedPassword));
            if (result == mysql::DbOperatorStatus::Success) [[likely]] {
                nlohmann::json dataObj;
                dataObj["request_type"] = static_cast<int>(Flicker::Client::Enums::ServiceType::ResetPassword);
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::ok);
                responseRoot["message"] = "Reset password successful!";
                responseRoot["data"] = dataObj;
                httpResponse.result(boost::beast::http::status::ok);
            }
            else [[unlikely]] {
                responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::service_unavailable);
                responseRoot["message"] = "The service is temporarily unavailable while the server is under maintenance!";
                httpResponse.result(boost::beast::http::status::service_unavailable);
            }
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("重置密码服务回调执行发生异常: {}", e.what()));
            responseRoot["response_status_code"] = static_cast<int>(boost::beast::http::status::internal_server_error);
            responseRoot["message"] = "Server Internal Error!";
            httpResponse.result(boost::beast::http::status::internal_server_error);
        }

        boost::beast::ostream(httpResponse.body()) << responseRoot.dump();
        };

    this->registerCallback("/get_verify_code", boost::beast::http::verb::post, getVerifyCodeFunc);
    this->registerCallback("/login_user", boost::beast::http::verb::post, loginUserFunc);
    this->registerCallback("/register_user", boost::beast::http::verb::post, registerUserFunc);
    this->registerCallback("/authenticate_reset_pwd", boost::beast::http::verb::post, authenticateResetPwdFunc);
    this->registerCallback("/reset_password", boost::beast::http::verb::post, resetPasswordFunc);

}

bool FKLogicSystem::callBack(const std::string& url, boost::beast::http::verb requestType, std::shared_ptr<FKHttpConnection> connection)
{
    if (requestType == boost::beast::http::verb::get) {
        auto it = _pGetRequestCallBacks.find(url);
        if (it == _pGetRequestCallBacks.end()) {
            LOGGER_ERROR(std::format("服务器未注册当前服务的GET处理函数: {}", url));
            return false;
        }

        LOGGER_INFO(std::format("执行GET处理函数: {}", url));
        it->second(connection);
    }
    else if (requestType == boost::beast::http::verb::post) {
        auto it = _pPostRequestCallBacks.find(url);
        if (it == _pPostRequestCallBacks.end()) {
            LOGGER_ERROR(std::format("服务器未注册当前服务的POST处理函数: {}", url));
            return false;
        }

        LOGGER_INFO(std::format("执行POST处理函数: {}", url));
        it->second(connection);
    }
    else {
        return false;
    }

    return true;
}

void FKLogicSystem::registerCallback(const std::string& url, boost::beast::http::verb requestType, MessageHandler handler)
{
    // 检查参数有效性
    if (url.empty()) {
        LOGGER_ERROR("尝试注册空URL的回调函数");
        return;
    }

    if (!handler) {
        LOGGER_ERROR(std::format("尝试注册空回调函数, URL: {}", url));
        return;
    }

    if (requestType == boost::beast::http::verb::get) {
        if (_pGetRequestCallBacks.find(url) != _pGetRequestCallBacks.end()) {
            LOGGER_WARN(std::format("覆盖已存在的GET回调函数, URL: {}", url));
        }

        _pGetRequestCallBacks[url] = handler;
        LOGGER_INFO(std::format("成功注册GET回调函数: {}", url));
    }
    else if (requestType == boost::beast::http::verb::post) {
        if (_pPostRequestCallBacks.find(url) != _pPostRequestCallBacks.end()) {
            LOGGER_WARN(std::format("覆盖已存在的POST回调函数, URL: {}", url));
        }

        _pPostRequestCallBacks[url] = handler;
        LOGGER_INFO(std::format("成功注册POST回调函数: {}", url));
    }
    else {

    }
}