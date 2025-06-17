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
#include <NXDef.h>

namespace LoginSide {
	enum OperationType {
		Login = 0x0001,
		Register = 0x0002,
		ForgetPassword = 0x0003,
		QrCodeLogin = 0x0004
	};
};

namespace Http {
	enum RequestId {
		ID_GET_VARIFY_CODE = 1001,
		ID_REGISTER_USER = 1002,
	};
	enum RequestSeviceType {
		REGISTER_SERVICE,
	};
	enum RequestErrorCode {
		SUCCESS,
		PARSER_JSON_ERROR,
		NETWORK_ERROR,
	};
}

#endif // !FK_DEF_H_
