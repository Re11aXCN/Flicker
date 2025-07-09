/*************************************************************************************
 *
 * @ Filename	 : FKDef.h
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
#ifndef FK_DEF_H_
#define FK_DEF_H_
#include <magic_enum/magic_enum.hpp>
namespace Http {
	enum class RequestType {
		GET,
		POST
	};
	enum class RequestId {
		ID_GET_VERIFY_CODE = 1001,
		ID_REGISTER_USER = 1002,
		ID_LOGIN_USER = 1003,
		ID_RESET_PASSWORD = 1004
	};
	enum class RequestSeviceType {
		GET_VERIFY_CODE,
		REGISTER_USER,
		LOGIN_USER,
		RESET_PASSWORD,
	};
	enum class RequestStatusCode {
		SUCCESS				= 0,
		INVALID_JSON		= 101,
		NETWORK_ABNORMAL	= 102,
		GRPC_CALL_FAILED	= 103,
		MISSING_FIELDS		= 104,
		VERIFY_CODE_EXPIRED	= 105,
		VERIFY_CODE_ERROR	= 106,
		USER_EXIST			= 107,
		NOT_FIND_USER		= 108,
		DATABASE_ERROR		= 109,
		PASSWORD_ERROR		= 110,
	};
}

namespace gRPC {
	enum class ServiceType {
		VERIFY_CODE_SERVICE = 0,  // 验证码服务
		PASSWORD_SERVICE,
	};
}

namespace DbOperator {
	// 用户注册结果枚举
	enum class UserRegisterResult {
		SUCCESS,                // 注册成功
		USERNAME_EXISTS,        // 用户名已存在
		EMAIL_EXISTS,           // 邮箱已存在
		DATABASE_ERROR,         // 数据库错误
		INVALID_PARAMETERS      // 参数无效
	};
}

#ifdef QT_CORE_LIB
#include <QObject>
#include <QFlags>
namespace Launcher {
	// 输入验证状态枚举
	enum class InputValidationFlag {
		None = 0x00,
		UsernameValid = 0x01,
		EmailValid = 0x02,
		PasswordValid = 0x04,
		ConfirmPasswordValid = 0x08,
		VerifyCodeValid = 0x10,
		AllLoginValid = UsernameValid | PasswordValid,
		AllRegisterValid = UsernameValid | EmailValid | PasswordValid | ConfirmPasswordValid | VerifyCodeValid,
		AllAuthenticationValid = EmailValid | VerifyCodeValid,
		AllResetPasswordValid = PasswordValid | ConfirmPasswordValid,
	};
	Q_DECLARE_FLAGS(InputValidationFlags, InputValidationFlag)
	Q_DECLARE_OPERATORS_FOR_FLAGS(InputValidationFlags)

	enum class FormType {
		Login,
		Register,
		Authentication,
		ResetPassword,
	};
};

#endif

template <>
struct magic_enum::customize::enum_range<Http::RequestStatusCode> {
	static constexpr int min = 0;  // 最小枚举值
	static constexpr int max = 256;  // 最大枚举值
	// 注意：max - min 必须小于 UINT16_MAX
};

template <>
struct magic_enum::customize::enum_range<Http::RequestId> {
	static constexpr int min = 1000;  // 最小枚举值
	static constexpr int max = 1100;  // 最大枚举值
	// 注意：max - min 必须小于 UINT16_MAX
};

#endif // !FK_DEF_H_
