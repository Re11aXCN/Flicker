#ifndef FK_EMAIL_SENDER_H_
#define FK_EMAIL_SENDER_H_

#include <memory>
#include "universal/utils.h"
#include "universal/smtp/smtp_mail_sender.h"
#include "Library/Logger/logger.h"
#include "FKDef.h"
#include "FKEmailTemplate.h"

class FKEmailSender {
public:
    static bool sendVerificationEmail(
        const std::string& email,
        const std::string& code,
        Flicker::Client::Enums::ServiceType serviceType)
    {
        using namespace universal::utils::path;
        // 获取模板
        static auto result = FKEmailTemplate::create(
            get_nth_parent_directory(get_executable_path().value(), 3).value()
            / std::filesystem::path(
                universal::utils::string::join(
                universal::utils::path::local_separator(),
                "Flicker", 
                "Global",
                "Smtp",
                "template.html")), { "{{VERIFICATION_TYPE}}", "{{VERIFICATION_CODE}}" });
        if (!result) {
            FileError error = result.error();
            LOGGER_ERROR(std::format("获取预处理模板失败, message: {}, code: {}", error.message, error.code));
            return false;
        }
        auto template_ = result.value();
        bool is_register = (serviceType == Flicker::Client::Enums::ServiceType::Register);
        // 生成邮件内容
        auto email_body = template_.generate({ is_register ? "注册" : "重置密码", code});
        if (!email_body) {
            LOGGER_ERROR(std::format("参数错误: {}", magic_enum::enum_name(email_body.error())));
            return false;
        }
        // 配置邮件信息
        universal::smtp::EmailInfo verify_email;
        verify_email.smtp_url = "smtp://smtp.qq.com:465";
        verify_email.username = "2634544095@qq.com";
        verify_email.password = "jdodjyndaadmeadc";
        verify_email.from = "2634544095@qq.com";
        verify_email.to = email;
        verify_email.subject = is_register
            ? "Flicker 注册验证码"
            : "Flicker 密码重置验证码";
        verify_email.is_html = true;
        verify_email.body = std::move(email_body.value());

        // 发送邮件
        return universal::smtp::send_email(verify_email);
    }
};
#endif // !FK_EMAIL_SENDER_H_