/*************************************************************************************
 *
 * @ Filename     : FKDef.h
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
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
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
namespace flicker = boost::beast;
namespace boost::beast {
    namespace http {
        enum class service : uint16_t {
            VerifyCode,
            Login,
            Register,
            ResetPassword,
            AuthenticateUser,
        };
        /*
        enum class ResponseStatusCode : uint16_t {
            SUCCESS, // status::ok (200)    请求成功处理
            INVALID_JSON,//status::bad_request (400)    JSON 格式错误属于客户端请求错误
            NETWORK_ABNORMAL,//status::bad_request (400)    缺少必要字段属于客户端请求不完整
            GRPC_CALL_FAILED,//status::unauthorized (401)    验证凭证过期需要重新认证
            MISSING_FIELDS,//status::unauthorized (401)    验证凭证错误需要重新认证
            VERIFY_CODE_EXPIRED,//status::unauthorized (401)    密码错误属于认证失败
            VERIFY_CODE_ERROR,//status::conflict (409)    资源冲突（如重复创建用户）
            USER_EXIST,//status::not_found (404)    资源未找到
            NOT_FIND_USER,//status::internal_server_error (500)    数据库错误属于服务器内部问题
            DATABASE_ERROR,//status::bad_gateway (502)    下游服务调用失败（类似网关错误）
            PASSWORD_ERROR,//status::service_unavailable (503)    网络异常导致服务不可用
        };
        */
    }
    namespace grpc {
        enum class service : uint16_t {
            VerifyCode,
            EncryptPassword,
            AuthenticatePwdReset,
        };
    }
}

namespace DbOperator {
    enum class Status {
        Success, 
        DataAlreadyExist,
        DataNotExist,
        DatabaseError,
        InvalidParameters
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
struct magic_enum::customize::enum_range<flicker::http::status> {
    static constexpr int min = 0;  // 最小枚举值
    static constexpr int max = 511;  // 最大枚举值
    // 注意：max - min 必须小于 UINT16_MAX
};
#endif // !FK_DEF_H_
