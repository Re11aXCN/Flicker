#include "smtp_mail_sender.h"
#include <iostream>
#include <filesystem>

using namespace universal;
inline std::filesystem::path file_dir{"C:\\Users\\Re11a\\Desktop"};

// 1. 发送纯文本邮件
inline void send_text_email(const smtp::EmailInfo& base_email) {
    smtp::EmailInfo text_email = base_email;
    text_email.is_html = false;
    text_email.body = "这是一封测试邮件，包含纯文本内容。";
    
    // 添加一个文本附件
    auto text_attachment = smtp::create_attachment(file_dir / std::filesystem::path("document.txt"));
    if (text_attachment) {
        text_email.attachments.push_back(*text_attachment);
    }
    
    // 添加一个PDF附件
    auto pdf_attachment = smtp::create_attachment(file_dir / std::filesystem::path("document.pdf"));
    if (pdf_attachment) {
        text_email.attachments.push_back(*pdf_attachment);
    }
    
    if (smtp::send_email(text_email)) {
        std::cout << "纯文本邮件发送成功！" << std::endl;
    } else {
        std::cerr << "纯文本邮件发送失败！" << std::endl;
    }
}

// 2. 发送HTML邮件带内联图片
inline void send_html_email_with_images(const smtp::EmailInfo& base_email) {
    smtp::EmailInfo html_email = base_email;
    html_email.is_html = true;
    
    // 添加内联图片
    auto inline_image1 = smtp::create_inline_image(file_dir / std::filesystem::path("image1.jpg"), "image1");
    auto inline_image2 = smtp::create_inline_image(file_dir / std::filesystem::path("image2.png"), "image2");
    
    if (inline_image1) {
        html_email.inline_images.push_back(*inline_image1);
    }
    
    if (inline_image2) {
        html_email.inline_images.push_back(*inline_image2);
    }
    
    // HTML内容中引用内联图片
    html_email.body = R"(
    <html>
    <body>
        <h1>测试HTML邮件</h1>
        <p>这是一封测试邮件，包含HTML内容和内联图片。</p>
        <p>图片1：</p>
        <img src="cid:image1" alt="图片1" style="max-width:100%;"/>
        <p>图片2：</p>
        <img src="cid:image2" alt="图片2" style="max-width:100%;"/>
    </body>
    </html>
    )";
    
    // 添加一个Office文档附件
    auto doc_attachment = smtp::create_attachment(file_dir / std::filesystem::path("document.docx"));
    if (doc_attachment) {
        html_email.attachments.push_back(*doc_attachment);
    }
    
    // 添加一个压缩包附件
    auto zip_attachment = smtp::create_attachment(file_dir / std::filesystem::path("archive.zip"));
    if (zip_attachment) {
        html_email.attachments.push_back(*zip_attachment);
    }
    
    if (smtp::send_email(html_email)) {
        std::cout << "HTML邮件发送成功！" << std::endl;
    } else {
        std::cerr << "HTML邮件发送失败！" << std::endl;
    }
}

// 3. 从内存创建附件示例
inline void send_email_with_memory_attachment(const smtp::EmailInfo& base_email) {
    smtp::EmailInfo memory_email = base_email;
    memory_email.is_html = true;
    memory_email.body = "<html><body><h1>测试从内存创建附件</h1><p>这封邮件包含从内存创建的附件。</p></body></html>";
    
    // 创建一些示例数据
    std::vector<std::byte> sample_data(1024, std::byte{65}); // 1KB的'A'字符
    
    // 从内存创建附件
    auto memory_attachment = smtp::create_attachment_from_memory(
        "memory_file.txt",
        std::span<const std::byte>(sample_data.data(), sample_data.size()),
        "text/plain",
        ""
    );
    
    memory_email.attachments.push_back(memory_attachment);
    
    if (smtp::send_email(memory_email)) {
        std::cout << "带内存附件的邮件发送成功！" << std::endl;
    } else {
        std::cerr << "带内存附件的邮件发送失败！" << std::endl;
    }
}

inline int example_smtp_mail_sender() {
    // 基本邮件信息
    smtp::EmailInfo email;
    email.smtp_url = "smtp://smtp.qq.com:465";
    email.username = "2634544095@qq.com";  // 替换为你的邮箱账号
    email.password = "jdodjyndaadmeadc";   // 替换为你的邮箱授权码
    email.from = "2634544095@qq.com";      // 替换为你的邮箱
    email.to = "2634544095@qq.com";    // 替换为收件人邮箱
    
    // 添加抄送和密送
    email.cc.push_back("2081973889@qq.com");
    //email.bcc.push_back("2634544095@example.com");
    
    email.subject = "测试带附件和内联图片的邮件";
    
    // 运行示例
    send_text_email(email);
    send_html_email_with_images(email);
    send_email_with_memory_attachment(email);
    
    return 0;
}