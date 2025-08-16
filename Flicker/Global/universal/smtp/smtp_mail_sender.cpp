#include "smtp_mail_sender.h"
#include <curl/curl.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <format>

namespace universal::smtp {

    // Base64编码函数
    void base64_encode(std::span<const std::byte> input, std::string& output) {
        // Base64编码所需的字符表
        static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const unsigned char* data = reinterpret_cast<const unsigned char*>(input.data());
        size_t input_length = input.size();
        
        // 计算编码后的长度（每3字节转为4字符，不足3的倍数部分需要特殊处理）
        size_t output_length = 4 * ((input_length + 2) / 3);
        output.clear();
        output.reserve(output_length);
        
        size_t i = 0;
        while (i < input_length) {
            // 将3个字节转换为4个Base64字符
            uint32_t octet_a = i < input_length ? data[i++] : 0;
            uint32_t octet_b = i < input_length ? data[i++] : 0;
            uint32_t octet_c = i < input_length ? data[i++] : 0;
            
            uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
            
            output.push_back(base64_chars[(triple >> 18) & 0x3F]);
            output.push_back(base64_chars[(triple >> 12) & 0x3F]);
            output.push_back(base64_chars[(triple >> 6) & 0x3F]);
            output.push_back(base64_chars[triple & 0x3F]);
        }
        
        // 处理填充
        size_t padding = 3 - (input_length % 3);
        if (padding == 3) padding = 0;
        
        for (size_t j = 0; j < padding; ++j) {
            output[output_length - 1 - j] = '=';
        }
        
        // 每76个字符添加一个换行符，符合MIME标准
        for (size_t j = 76; j < output.size(); j += 77) {
            output.insert(j, "\r\n");
        }
    }

    // 生成随机边界字符串
    std::string generate_boundary() {
        static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
        
        std::string boundary = "------------------------";
        for (int i = 0; i < 24; ++i) {
            boundary += charset[dist(gen)];
        }
        return boundary;
    }

    // 获取文件扩展名对应的MIME类型
    std::string get_mime_type(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // MIME类型映射表
        static const std::unordered_map<std::string, std::string> mime_types = {
            {".txt", "text/plain"},
            {".html", "text/html"},
            {".htm", "text/html"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".bmp", "image/bmp"},
            {".pdf", "application/pdf"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
            {".zip", "application/zip"},
            {".rar", "application/x-rar-compressed"},
            {".7z", "application/x-7z-compressed"},
            {".tar", "application/x-tar"},
            {".gz", "application/gzip"},
            {".mp3", "audio/mpeg"},
            {".mp4", "video/mp4"},
            {".avi", "video/x-msvideo"},
            {".mov", "video/quicktime"},
            {".csv", "text/csv"},
            {".json", "application/json"},
            {".xml", "application/xml"},
            {".svg", "image/svg+xml"}
        };
        
        auto it = mime_types.find(ext);
        if (it != mime_types.end()) {
            return it->second;
        }
        return "application/octet-stream"; // 默认二进制类型
    }

    // 从文件创建附件
    std::optional<Attachment> Attachment::from_file(const std::filesystem::path& path, const std::string& content_id) {
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            return std::nullopt;
        }
        
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return std::nullopt;
        }
        
        // 获取文件大小
        auto file_size = std::filesystem::file_size(path);
        
        // 读取文件内容
        std::vector<std::byte> data(file_size);
        file.read(reinterpret_cast<char*>(data.data()), file_size);
        
        if (file.gcount() != static_cast<std::streamsize>(file_size)) {
            return std::nullopt;
        }
        
        Attachment attachment;
        attachment.filename = path.filename().string();
        attachment.content_type = get_mime_type(path);
        attachment.data = std::move(data);
        attachment.content_id = content_id;
        
        return attachment;
    }

    // 从文件创建内联图片
    std::optional<InlineImage> InlineImage::from_file(const std::filesystem::path& path, const std::string& content_id) {
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            return std::nullopt;
        }
        
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return std::nullopt;
        }
        
        // 获取文件大小
        auto file_size = std::filesystem::file_size(path);
        
        // 读取文件内容
        std::vector<std::byte> data(file_size);
        file.read(reinterpret_cast<char*>(data.data()), file_size);
        
        if (file.gcount() != static_cast<std::streamsize>(file_size)) {
            return std::nullopt;
        }
        
        // 检查是否为图片类型
        std::string content_type = get_mime_type(path);
        if (content_type.find("image/") != 0) {
            return std::nullopt; // 不是图片类型
        }
        
        InlineImage image;
        image.filename = path.filename().string();
        image.content_type = std::move(content_type);
        image.data = std::move(data);
        image.content_id = content_id;
        
        return image;
    }

    // 从文件路径创建附件
    std::optional<Attachment> create_attachment(const std::filesystem::path& path, const std::string& content_id) {
        return Attachment::from_file(path, content_id);
    }

    // 从文件路径创建内联图片
    std::optional<InlineImage> create_inline_image(const std::filesystem::path& path, const std::string& content_id) {
        return InlineImage::from_file(path, content_id);
    }

    // 从内存数据创建附件
    Attachment create_attachment_from_memory(const std::string& filename, 
                                           std::span<const std::byte> data,
                                           const std::string& content_type,
                                           const std::string& content_id) {
        Attachment attachment;
        attachment.filename = filename;
        attachment.content_type = content_type;
        attachment.data.assign(data.begin(), data.end());
        attachment.content_id = content_id;
        return attachment;
    }

    // 从内存数据创建内联图片
    InlineImage create_inline_image_from_memory(const std::string& filename,
                                              std::span<const std::byte> data,
                                              const std::string& content_type,
                                              const std::string& content_id) {
        InlineImage image;
        image.filename = filename;
        image.content_type = content_type;
        image.data.assign(data.begin(), data.end());
        image.content_id = content_id;
        return image;
    }

    struct UploadStatus {
        std::size_t bytes_read = 0;
        std::string payload;
    };

    static std::size_t payload_source(char* ptr, std::size_t size, std::size_t nmemb, void* userp) {
        UploadStatus* upload_ctx = static_cast<UploadStatus*>(userp);
        const std::size_t buffer_size = size * nmemb;

        if (upload_ctx->bytes_read >= upload_ctx->payload.size())
            return 0;

        std::size_t copy_size = upload_ctx->payload.size() - upload_ctx->bytes_read;
        if (copy_size > buffer_size)
            copy_size = buffer_size;

        memcpy(ptr, upload_ctx->payload.c_str() + upload_ctx->bytes_read, copy_size);
        upload_ctx->bytes_read += copy_size;
        return copy_size;
    }

    bool send_email(const EmailInfo& info) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        CURLcode res;
        curl_slist* recipients = nullptr;

        // 生成MIME边界
        std::string boundary = generate_boundary();
        
        // 构造邮件头部
        std::string payload;
        payload += "To: " + info.to + "\r\n";
        payload += "From: " + info.from + "\r\n";
        
        // 添加抄送收件人
        if (!info.cc.empty()) {
            payload += "Cc: ";
            for (size_t i = 0; i < info.cc.size(); ++i) {
                if (i > 0) payload += ", ";
                payload += info.cc[i];
            }
            payload += "\r\n";
        }
        
        payload += "Subject: " + info.subject + "\r\n";
        payload += "MIME-Version: 1.0\r\n";
        
        // 如果有附件或内联图片，使用multipart格式
        if (!info.attachments.empty() || !info.inline_images.empty()) {
            payload += std::format("Content-Type: multipart/mixed; boundary=\"{}\"\r\n", boundary);
            payload += "\r\n";
            
            // 如果有内联图片，使用multipart/related
            if (!info.inline_images.empty()) {
                std::string related_boundary = generate_boundary();
                payload += std::format("--{}\r\n", boundary);
                payload += std::format("Content-Type: multipart/related; boundary=\"{}\"\r\n", related_boundary);
                payload += "\r\n";
                
                // 添加正文部分
                payload += std::format("--{}\r\n", related_boundary);
                if (info.is_html) {
                    payload += "Content-Type: text/html; charset=UTF-8\r\n";
                } else {
                    payload += "Content-Type: text/plain; charset=UTF-8\r\n";
                }
                payload += "Content-Transfer-Encoding: 8bit\r\n";
                payload += "\r\n";
                payload += info.body;
                payload += "\r\n";
                
                // 添加内联图片
                for (const auto& image : info.inline_images) {
                    payload += std::format("--{}\r\n", related_boundary);
                    payload += std::format("Content-Type: {}\r\n", image.content_type);
                    payload += "Content-Transfer-Encoding: base64\r\n";
                    payload += std::format("Content-Disposition: inline; filename=\"{}\"\r\n", image.filename);
                    if (!image.content_id.empty()) {
                        payload += std::format("Content-ID: <{}>\r\n", image.content_id);
                    }
                    payload += "\r\n";
                    
                    // Base64编码图片数据
                    std::string base64_output;
                    base64_encode(std::span<const std::byte>(image.data.data(), image.data.size()), base64_output);
                    payload += base64_output;
                    payload += "\r\n";
                }
                
                payload += std::format("--{}--\r\n", related_boundary);
            } else {
                // 没有内联图片，直接添加正文
                payload += std::format("--{}\r\n", boundary);
                if (info.is_html) {
                    payload += "Content-Type: text/html; charset=UTF-8\r\n";
                } else {
                    payload += "Content-Type: text/plain; charset=UTF-8\r\n";
                }
                payload += "Content-Transfer-Encoding: 8bit\r\n";
                payload += "\r\n";
                payload += info.body;
                payload += "\r\n";
            }
            
            // 添加附件
            for (const auto& attachment : info.attachments) {
                payload += std::format("--{}\r\n", boundary);
                payload += std::format("Content-Type: {}; name=\"{}\"\r\n", 
                                      attachment.content_type, attachment.filename);
                payload += "Content-Transfer-Encoding: base64\r\n";
                payload += std::format("Content-Disposition: attachment; filename=\"{}\"\r\n", 
                                      attachment.filename);
                if (!attachment.content_id.empty()) {
                    payload += std::format("Content-ID: <{}>\r\n", attachment.content_id);
                }
                payload += "\r\n";
                
                // Base64编码附件数据
                std::string base64_output;
                base64_encode(std::span<const std::byte>(attachment.data.data(), attachment.data.size()), base64_output);
                payload += base64_output;
                payload += "\r\n";
            }
            
            // 结束multipart
            payload += std::format("--{}--\r\n", boundary);
        } else {
            // 没有附件，使用普通格式
            if (info.is_html) {
                payload += "Content-Type: text/html; charset=UTF-8\r\n";
            } else {
                payload += "Content-Type: text/plain; charset=UTF-8\r\n";
            }
            payload += "\r\n";
            payload += info.body;
            payload += "\r\n";
        }

        UploadStatus upload_ctx = { 0, payload };

        curl_easy_setopt(curl, CURLOPT_URL, info.smtp_url.c_str());                 // 设置SMTP服务器地址
        curl_easy_setopt(curl, CURLOPT_PORT, 587L);                                 // 设置SMTP端口
        curl_easy_setopt(curl, CURLOPT_USERNAME, info.username.c_str());            // 设置SMTP用户名
        curl_easy_setopt(curl, CURLOPT_PASSWORD, info.password.c_str());            // 设置SMTP密码（授权码）
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + info.from + ">").c_str()); // 设置发件人邮箱
        recipients = curl_slist_append(nullptr, ("<" + info.to + ">").c_str());     // 构造收件人邮箱列表
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);                      // 设置收件人邮箱
        
        // 添加抄送收件人
        for (const auto& cc_addr : info.cc) {
            recipients = curl_slist_append(recipients, ("<" + cc_addr + ">").c_str());
        }
        
        // 添加密送收件人
        for (const auto& bcc_addr : info.bcc) {
            recipients = curl_slist_append(recipients, ("<" + bcc_addr + ">").c_str());
        }
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);                    // 启用SSL/TLS加密传输               
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);                         // 跳过SSL证书校验
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);                         // 跳过SSL主机名校验
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");                // 指定 LOGIN 认证方式
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);               // 设置回调函数，用于提供邮件内容读取
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);                      // 设置回调函数的上下文数据，传递upload_ctx
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                                 // 设置为上传模式（即发邮件）
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                                // 开启调试信息输出

        res = curl_easy_perform(curl);                                              // 正式执行SMTP流程（连接、认证、发信）。libcurl会自动完成所有SMTP细节
        curl_slist_free_all(recipients);                                            // 释放收件人列表内存
        curl_easy_cleanup(curl);                                                    // 清理CURL句柄

        if (res != CURLE_OK) {
            std::cerr << "❌ curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        return true;
    }

} // namespace smtp
