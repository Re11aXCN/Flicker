#include "FKGrpcServicePoolManager.h"
#include <magic_enum/magic_enum.hpp>
#include "Source/FKConfigManager.h"

//namespace gRPC {
//	/**
//	* 使用bcrypt对密码进行哈希处理
//	* @param password 原始密码
//	* @return 返回一个元组，包含哈希后的密码和盐值
//	*/
//	static std::tuple<std::string, std::string> HashPassword(const std::string& password) {
//		try {
//			// 创建请求对象
//			FKPasswordGrpc::HashPasswordRequestBody request;
//			request.set_password(password);
//
//			// 创建响应对象
//			FKPasswordGrpc::HashPasswordResponseBody response;
//
//			// 创建gRPC上下文
//			grpc::ClientContext context;
//
//			// 设置超时时间（5秒）
//			std::chrono::system_clock::time_point deadline =
//				std::chrono::system_clock::now() + std::chrono::milliseconds(5);
//			context.set_deadline(deadline);
//
//			// 创建存根并调用服务
//			auto stub = CreateStub();
//			grpc::Status status = stub->HashPassword(&context, request, &response);
//
//			// 检查调用状态
//			if (status.ok()) {
//				// 调用成功，检查业务状态码
//				if (response.status_code() == 0) { // SUCCESS
//					std::println("密码加密成功");
//					return { response.hashed_password(), response.salt() };
//				}
//				else {
//					std::println("密码加密业务错误: {}", response.message());
//					return { "", "" };
//				}
//			}
//			else {
//				// gRPC调用失败
//				std::println("密码加密gRPC调用失败: {}, {}",
//					status.error_code(), status.error_message());
//				return { "", "" };
//			}
//		}
//		catch (const std::exception& e) {
//			std::println("密码加密异常: {}", e.what());
//			return { "", "" };
//		}
//	}
//
//	/**
//	 * 验证密码是否匹配
//	 * @param password 待验证的密码
//	 * @param hashedPassword 数据库中存储的哈希密码
//	 * @return 如果密码匹配返回true，否则返回false
//	 */
//	static bool verifyPassword(const std::string& password, const std::string& hashedPassword) {
//		try {
//			// 创建请求对象
//			FKPasswordGrpc::VerifyPasswordRequestBody request;
//			request.set_password(password);
//			request.set_hashed_password(hashedPassword);
//
//			// 创建响应对象
//			FKPasswordGrpc::VerifyPasswordResponseBody response;
//
//			// 创建gRPC上下文
//			grpc::ClientContext context;
//
//			// 设置超时时间（5秒）
//			std::chrono::system_clock::time_point deadline =
//				std::chrono::system_clock::now() + std::chrono::milliseconds(5);
//			context.set_deadline(deadline);
//
//			// 创建存根并调用服务
//			auto stub = CreateStub();
//			grpc::Status status = stub->VerifyPassword(&context, request, &response);
//
//			// 检查调用状态
//			if (status.ok()) {
//				// 调用成功，检查业务状态码
//				if (response.status_code() == 0) { // SUCCESS
//					std::println("密码验证完成，结果: {}", response.is_valid() ? "有效" : "无效");
//					return response.is_valid();
//				}
//				else {
//					std::println("密码验证业务错误: {}", response.message());
//					return false;
//				}
//			}
//			else {
//				// gRPC调用失败
//				std::println("密码验证gRPC调用失败: {}, {}",
//					status.error_code(), status.error_message());
//				return false;
//			}
//		}
//		catch (const std::exception& e) {
//			std::println("密码验证异常: {}", e.what());
//			return false;
//		}
//	}
//
//}

SINGLETON_CREATE_SHARED_CPP(FKGrpcServicePoolManager)

FKGrpcServicePoolManager::FKGrpcServicePoolManager() {
    std::println("FKGrpcServicePoolManager 已创建");
    
    // 初始化验证码服务
    try {
		_initializeService(gRPC::ServiceType::VERIFY_CODE_SERVICE);
		_initializeService(gRPC::ServiceType::PASSWORD_SERVICE);
    } catch (const std::exception& e) {
        std::println("初始化验证码服务失败: {}", e.what());
    }
}

FKGrpcServicePoolManager::~FKGrpcServicePoolManager() {
    shutdownAllServices();
}

void FKGrpcServicePoolManager::_initializeService(gRPC::ServiceType serviceType) {
#define GRPC_SERVICE_CASE(type, name)  case type: { \
        using service_type = ServiceTraits<type>::Type; \
        std::unique_ptr<FKGrpcConnectionPool<service_type>> pool( \
            new FKGrpcConnectionPool<service_type>(configManager->getGrpcServiceConfig(type)) \
        ); \
        auto rawPool = pool.release(); \
        _pServicePools[type] = { \
           .poolPtr = rawPool, \
           .shutdown = [rawPool] { \
                rawPool->shutdown(); \
                delete rawPool; \
            }, \
           .getStatus = [rawPool] { \
                return rawPool->getStatus(); \
            } \
        }; \
        std::println("已初始化{}服务连接池", name); \
        break; \
    }

    std::lock_guard<std::mutex> lock(_pMutex);
    if (_pServicePools.find(serviceType) != _pServicePools.end()) {
        std::println("服务已初始化，请先关闭再重新初始化");
        return;
    }

	FKConfigManager* configManager = FKConfigManager::getInstance();
    try {
        switch (serviceType) {
			GRPC_SERVICE_CASE(gRPC::ServiceType::VERIFY_CODE_SERVICE, "验证码")
			GRPC_SERVICE_CASE(gRPC::ServiceType::PASSWORD_SERVICE, "密码")
            default: throw std::runtime_error("未知的服务类型");
        }
    }
    catch (const std::exception& e) {
        std::println("初始化服务失败: {}", e.what());
        throw;
    }
#undef GRPC_SERVICE_CASE
}

void FKGrpcServicePoolManager::shutdownService(gRPC::ServiceType serviceType) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    auto it = _pServicePools.find(serviceType);
    if (it != _pServicePools.end()) {
		// 先关闭连接池
		it->second.shutdown();
		// 然后移除
		_pServicePools.erase(it);
        std::println("已关闭服务连接池: {}", static_cast<int>(serviceType));
    }
}

void FKGrpcServicePoolManager::shutdownAllServices() {
    std::lock_guard<std::mutex> lock(_pMutex);
	for (auto& [type, wrapper] : _pServicePools) {
		wrapper.shutdown();
	}
    _pServicePools.clear();
    std::println("已关闭所有服务连接池");
}

std::string FKGrpcServicePoolManager::getAllServicesStatus() const {
    std::lock_guard<std::mutex> lock(_pMutex);
	std::ostringstream oss;

	oss << "gRPC Services Status:\n";
	oss << "================================\n";

	for (const auto& [type, wrapper] : _pServicePools) {
		oss << "[" << magic_enum::enum_name(type) << "]: "
			<< wrapper.getStatus() << "\n";
	}

	if (_pServicePools.empty()) {
		oss << "No active services\n";
	}

	oss << "================================\n";
	return oss.str();
}
