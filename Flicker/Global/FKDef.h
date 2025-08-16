#ifndef FK_DEF_H_
#define FK_DEF_H_
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include "universal/macros.h"
#include "universal/flags.h"
namespace Flicker {
    namespace Server {
        namespace Enums {
            enum class GrpcServiceType : uint16_t {
                AuthenticateLogin,
                GenerateToken,
                ValidateToken,
            };
        }
    }
    namespace Client {
        namespace Enums {
            enum class ServiceType : uint16_t {
                VerifyCode,
                Login,
                Register,
                ResetPassword,
                AuthenticateResetPwd,
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
    }
    namespace Tcp {
#pragma pack(push, 1)
        // 消息头结构
        struct MessageHeader {
            uint64_t timestamp;  // 时间戳
            uint32_t magic;      // 魔数 0x464B4348 ("FKCH")
            uint32_t length;     // 消息体长度
            uint16_t type;       // 消息类型
            uint16_t version;    // 协议版本
            uint32_t reserved;   // 预留字段
        };
#pragma pack(pop)
        // 消息类型枚举
        enum class MessageType : uint16_t {
            HEARTBEAT = 1,           // 心跳
            AUTH_REQUEST = 2,        // 认证请求
            AUTH_RESPONSE = 3,       // 认证响应
            CHAT_MESSAGE = 4,        // 聊天消息
            USER_STATUS = 5,         // 用户状态
            SYSTEM_NOTIFICATION = 6, // 系统通知
            ERROR_MESSAGE = 7        // 错误消息
        };
        // 连接状态枚举
        enum class ConnectionState : uint16_t {
            // 基础状态
            DISCONNECTED = 0x0001,    // 1 << 0   未连接
            CONNECTING = 0x0002,      // 1 << 1   连接中  
            CONNECTED = 0x0004,       // 1 << 2   已连接
            // 附加标志 (可组合，必须在CONNECTED状态下)
            AUTHENTICATING = 0x0008,  // 1 << 3   认证中
            AUTHENTICATED = 0x0010,   // 1 << 4   已认证
            ERROR_STATE = 0x0020,     // 1 << 5   错误状态
        };

        // 声明ConnectionStates类型
        DECLARE_FLAGS(ConnectionStates, ConnectionState)
        DECLARE_OPERATORS_FOR_FLAGS(ConnectionStates)
    }
}

#ifdef QT_CORE_LIB
#include <QObject>
#include <QColor>
#include <QFlags>
namespace Flicker {
    namespace Client {
        // 输入验证状态枚举
        namespace Enums {
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
        }
        namespace Constants {
            constexpr int WIDGET_WIDTH = 1000;
            constexpr int WIDGET_HEIGHT = 600;
            constexpr int SHELL_BOX_SHADOW_WIDTH = 10;
            constexpr int SHELL_BORDER_RADIUS = 12;
            constexpr int SWITCH_BOX_SHADOW_WIDTH = 10;
            constexpr QColor LIGHT_MAIN_BG_COLOR{ 235, 239, 243, 255 };
            constexpr QColor DARK_MAIN_BG_COLOR{ 235, 239, 243, 255 };
            constexpr QColor SWITCH_BOX_SHADOW_COLOR{ 209, 217, 230, 140 };
            constexpr QColor SWITCH_CIRCLE_DARK_SHADOW_COLOR{ 184, 190, 199, 36 };
            constexpr QColor SWITCH_CIRCLE_LIGHT_SHADOW_COLOR{ 0, 54, 180, 36 };
            constexpr QColor DESCRIPTION_TEXT_COLOR{ 160, 165, 168, 255 };
            constexpr QColor LAUNCHER_PUSHBTN_TEXT_COLOR{ 249, 249, 249, 255 };
            constexpr QColor LIGHT_TEXT_COLOR{ 249, 249, 249, 255 };
            constexpr QColor DARK_TEXT_COLOR{ 24, 24, 24, 255 };
            constexpr QColor LAUNCHER_ICON_NORMAL_COLOR{ 198, 203, 205, 255 };
            constexpr QColor LAUNCHER_ICON_HOVER_COLOR{ 160, 165, 168, 255 };
        }
    }
}
#endif

template <>
struct magic_enum::customize::enum_range<boost::beast::http::status> {
    static constexpr int min = 0;  // 最小枚举值
    static constexpr int max = 511;  // 最大枚举值
    // 注意：max - min 必须小于 UINT16_MAX
};
#endif // !FK_DEF_H_
