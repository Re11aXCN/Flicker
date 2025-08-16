#ifndef UNIVERSAL_SMTP_MAIL_SENDER_H_
#define UNIVERSAL_SMTP_MAIL_SENDER_H_
#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <span>

namespace universal::smtp {

    // 附件信息结构体
    struct Attachment {
        std::string filename;           // 附件文件名（显示在邮件中的名称）
        std::string content_type;       // 内容类型，如 application/pdf, image/jpeg 等
        std::vector<std::byte> data;    // 附件数据
        std::string content_id;         // 内容ID，用于在HTML中引用图片，如 <img src="cid:image1">
        
        // 从文件路径创建附件
        static std::optional<Attachment> from_file(const std::filesystem::path& path, 
                                                  const std::string& content_id = "");
    };
    
    // 内联图片结构体（特殊类型的附件，可在HTML中引用）
    struct InlineImage {
        std::string content_id;          // 内容ID，用于在HTML中引用，如 <img src="cid:image1">
        std::string filename;           // 文件名
        std::vector<std::byte> data;    // 图片数据
        std::string content_type;       // 内容类型，如 image/jpeg, image/png 等
        
        // 从文件路径创建内联图片
        static std::optional<InlineImage> from_file(const std::filesystem::path& path, 
                                                   const std::string& content_id);
    };

    struct EmailInfo {
        std::string smtp_url;            // SMTP地址，例如 smtp://smtp.qq.com:465
        std::string username;            // 邮箱账号
        std::string password;            // 邮箱密码（授权码）
        std::string from;                // 发件人邮箱
        std::string to;                  // 收件人邮箱
        std::vector<std::string> cc;     // 抄送邮箱列表
        std::vector<std::string> bcc;    // 密送邮箱列表
        std::string subject;             // 邮件主题
        std::string body;                // 邮件内容（纯文本或HTML）
        bool is_html = false;            // 是否HTML格式
        std::vector<Attachment> attachments;    // 附件列表
        std::vector<InlineImage> inline_images; // 内联图片列表
    };

    // 发送邮件，支持纯文本与HTML、附件和内联图片
    bool send_email(const EmailInfo& info);
    
    // 从文件路径创建附件
    std::optional<Attachment> create_attachment(const std::filesystem::path& path, const std::string& content_id = "");
    
    // 从文件路径创建内联图片
    std::optional<InlineImage> create_inline_image(const std::filesystem::path& path, const std::string& content_id);
    
    // 从内存数据创建附件
    Attachment create_attachment_from_memory(const std::string& filename, 
                                           std::span<const std::byte> data,
                                           const std::string& content_type,
                                           const std::string& content_id = "");
    
    // 从内存数据创建内联图片
    InlineImage create_inline_image_from_memory(const std::string& filename,
                                              std::span<const std::byte> data,
                                              const std::string& content_type,
                                              const std::string& content_id);

} // namespace smtp

#endif // !UNIVERSAL_SMTP_MAIL_SENDER_H_