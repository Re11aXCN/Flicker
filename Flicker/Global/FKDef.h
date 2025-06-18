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
	enum RequestId {
		ID_GET_VARIFY_CODE = 1001,
		ID_REGISTER_USER = 1002,
	};
	enum RequestSeviceType {
		REGISTER_SERVICE,
	};
	enum RequestErrorCode {
		SUCCESS				= 0,
		PARSER_JSON_ERROR	= 4001,
		NETWORK_ERROR		= 4002,
		RPC_ERROR			= 4003,
	};
}

#endif // !FK_DEF_H_
