#ifndef FK_CONSTANT_H_
#define FK_CONSTANT_H_
#ifdef QT_CORE_LIB
#include <QObject>
#include <QColor>
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

namespace Constant {
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
};

#endif
#endif // !FK_CONSTANT_H_
