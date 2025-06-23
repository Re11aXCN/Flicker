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

namespace LoginSide {
	enum OperationType {
		Login = 0x0000,
		Register = 0x0001,
		ForgetPassword = 0x0002,
		QrCodeLogin = 0x0003
	};
};

namespace Http {
	enum class RequestType {
		GET,
		POST
	};
	enum class RequestId {
		ID_GET_VARIFY_CODE = 1001,
		ID_REGISTER_USER = 1002,
		ID_LOGIN_USER = 1003,
	};
	enum class RequestSeviceType {
		GET_VARIFY_CODE,
		REGISTER_USER,
		LOGIN_USER,
		CHANGE_PASSWORD,
	};
	enum class RequestStatusCode {
		SUCCESS				= 0,
		INVALID_JSON		= 4001,
		NETWORK_ABNORMAL	= 4002,
		GRPC_CALL_FAILED	= 4003,
		MISSING_FIELDS 		= 4004,
		VERIFY_CODE_EXPIRED	= 4005,
		VERIFY_CODE_ERROR	= 4006,
		USER_EXIST			= 4007,
		DATABASE_ERROR 		= 4008,
		PASSWORD_ERROR		= 4010,
	};
}

namespace gRPC {
	enum class ServiceType {
		VERIFY_CODE_SERVICE = 0,  // 验证码服务
		USER_AUTH_SERVICE,        // 用户认证服务
		PROFILE_SERVICE,          // 用户资料服务
		MESSAGE_SERVICE,          // 消息服务

		SERVICE_TYPE_COUNT        // 服务类型计数，必须放在最后
	};
}

#endif // !FK_DEF_H_
