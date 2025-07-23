/*************************************************************************************
 *
 * @ Filename     : FKUtils.h
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
#ifndef FK_UTILS_H_
#define FK_UTILS_H_
#include <wchar.h>
#include <cctype> 
#include <array>
#include <algorithm>
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <cstdlib>
#ifdef QT_CORE_LIB
#include <QString>
#include <QColor>
#endif
#if defined(_WIN32)
#include <Windows.h>
#endif
namespace FKUtils {
    namespace helper {
        // 字符串长度计算辅助函数
        inline size_t string_length(const char* str) { return std::strlen(str); }
        inline size_t string_length(const std::string& str) { return str.length(); }
        inline size_t string_length(std::string_view str) { return str.length(); }
        
        // 字符串追加辅助函数
        inline void append_to_string(std::string& result, const char* str) { result.append(str); }
        inline void append_to_string(std::string& result, const std::string& str) { result.append(str); }
        inline void append_to_string(std::string& result, std::string_view str) { result.append(str); }
        
        template<typename T>
        inline auto append_to_string(std::string& result, const T& value) ->
            typename std::enable_if_t<std::is_arithmetic_v<T>, void> {
            result.append(std::to_string(value));
        }
//短整形高低字节交换
#define Swap16(A) ((((std::uint16_t)(A) & 0xff00) >> 8) | (((std::uint16_t)(A) & 0x00ff) << 8))
//长整形高低字节交换
#define Swap32(A) ((((std::uint32_t)(A) & 0xff000000) >> 24) | \
                   (((std::uint32_t)(A) & 0x00ff0000) >>  8) | \
                   (((std::uint32_t)(A) & 0x0000ff00) <<  8) | \
                   (((std::uint32_t)(A) & 0x000000ff) << 24))

/******************************************************************************
ENDIANNESS返回结果
    l:小端模式
    b:打断模式
******************************************************************************/
#define ENDIANNESS ((char)helper::endian_test.mylong)  

        static union {
            char c[4];
            unsigned long mylong;
        } endian_test = { { 'l', '?', '?', 'b' } };


        // 辅助函数：将半字节(0-15)转换为16进制字符（大写）
        constexpr unsigned char nibble_to_hex(unsigned char nibble) noexcept {
            nibble &= 0x0F; // 确保只有低4位
            return nibble < 10 ? ('0' + nibble) : ('A' + nibble - 10);
        }

        // 辅助函数：将16进制字符转换为半字节(0-15)
        constexpr unsigned char hex_to_nibble(unsigned char c) {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            throw std::invalid_argument("Invalid hexadecimal character");
        }

        // 编译期字符串视图（C++17起可用）
        template<size_t N>
        struct ConstexprString {
            char value[N];

            constexpr ConstexprString(const char(&str)[N]) {
                for (size_t i = 0; i < N; ++i) {
                    value[i] = str[i];
                }
            }

            constexpr operator const char* () const { return value; }
            constexpr const char* data() const { return value; }
            constexpr size_t size() const { return N - 1; } // 减去空字符
        };
    };

    /**
     * @brief : 将两个16进制字符解码为一个十进制字节
     * @usage :  unsigned char byte = hex_decode('1', 'A'); // 0x1A -> 26
     */
    template <typename T = unsigned char>
        requires std::same_as<T, unsigned char>
    constexpr T hex_decode(unsigned char high, unsigned char low) {
        return (helper::hex_to_nibble(high) << 4) | helper::hex_to_nibble(low);
    }

    /**
     * @brief : 将一个十进制字节编码为两个16进制字符（返回数组）
     * @usage : auto [high, low] = hex_encode(26u); // 返回 {'1', 'A'}
     */
    template <typename T>
        requires std::same_as<T, unsigned char>
    constexpr std::array<unsigned char, 2> hex_encode(T byte) noexcept {
        return {
            helper::nibble_to_hex(static_cast<unsigned char>(byte) >> 4), // 高4位
            helper::nibble_to_hex(byte)                                  // 低4位
        };
    }

    /**
     * @brief : 批量解码：将16进制数组转换为十进制字节数组
     * @usage : 
        std::vector<unsigned char> hex_data = {'A','1','F','F'};
        std::vector<unsigned char> decoded;
        hex_decode_bulk(hex_data.begin(), hex_data.end(), std::back_inserter(decoded));
        // decoded = [161, 255]
     */
    template <typename InputIt, typename OutputIt>
    constexpr void hex_decode_bulk(InputIt first, InputIt last, OutputIt out) {
        if (std::distance(first, last) % 2 != 0)
            throw std::invalid_argument("Hex data length must be even");

        while (first != last) {
            unsigned char high = *first++;
            unsigned char low = *first++;
            *out++ = hex_decode(high, low);
        }
    }

    /**
     * @brief : 批量编码：将十进制字节数组转换为16进制数组
     * @usage :
        std::vector<unsigned char> bytes = {10, 255};
        std::vector<unsigned char> encoded;
        hex_encode_bulk(bytes.begin(), bytes.end(), std::back_inserter(encoded));
        // encoded = {'0','A','F','F'}
     */
    template <typename InputIt, typename OutputIt>
    constexpr void hex_encode_bulk(InputIt first, InputIt last, OutputIt out) {
        while (first != last) {
            auto hex_pair = hex_encode(*first++);
            *out++ = hex_pair[0];
            *out++ = hex_pair[1];
        }
    }

    /**
     * @brief 高效拼接多个字符串
     * @tparam Args 可变参数类型
     * @param args 要拼接的字符串参数
     * @return 拼接后的字符串
     * 
     * 使用示例:
     * std::string result = concat("Hello", " ", "World", "!", 123);
     * // 结果: "Hello World!123"
     */
    template<typename... Args>
    inline std::string concat(Args&&... args) {
        // 计算总长度以预分配内存
        size_t totalSize = (0 + ... + helper::string_length(args));
        std::string result;
        result.reserve(totalSize);
        
        // 使用折叠表达式依次追加每个参数
        (helper::append_to_string(result, std::forward<Args>(args)), ...);
        return result;
    }

    /**
     * @brief 使用分隔符连接多个字符串
     * @tparam Args 可变参数类型
     * @param separator 分隔符
     * @param args 要连接的字符串参数
     * @return 连接后的字符串
     * 
     * 使用示例:
     * std::string result = join(", ", "apple", "banana", "orange", 42);
     * // 结果: "apple, banana, orange, 42"
     */
    template<typename Sep, typename... Args>
    inline std::string join(Sep&& separator, Args&&... args) {
        if constexpr (sizeof...(args) == 0) return std::string();
        
        // 计算分隔符数量和总长度
        const size_t separatorCount = sizeof...(args) - 1;
        const size_t separatorLength = helper::string_length(separator);
        const size_t totalSize = (0 + ... + helper::string_length(args)) + 
                                separatorCount * separatorLength;
        
        std::string result;
        result.reserve(totalSize);
        
        // 使用折叠表达式实现连接
        size_t index = 0;
        ((helper::append_to_string(result, (index++ > 0 ? separator : "")), 
          helper::append_to_string(result, std::forward<Args>(args))), ...);
        
        return result;
    }
    
    /**
     * @brief URL 编码（类似 Qt 的 toPercentEncoding）
     * @param input 要编码的字符串
     * @return URL 编码后的字符串
     *
     * 编码规则：
     * 1. 字母、数字、连字符(-)、下划线(_)、点(.)、波浪线(~) 不编码
     * 2. 空格编码为 '+' 或 "%20"（默认使用 %20）
     * 3. 其他字符编码为 %HH 格式（HH 是大写十六进制值）
     */
    inline std::string url_encode(std::string_view input, bool spaceAsPlus = false) {
        std::string result;
        // 预分配空间（最坏情况下每个字符变为3个字符）
        result.reserve(input.size() * 3);

        // 安全字符集（直接保留不编码）
        static const std::unordered_set<char> safe_chars = {
            '-', '_', '.', '~'
        };

        for (unsigned char c : input) {
            if (std::isalnum(c) || safe_chars.count(c)) {
                // 安全字符直接保留
                result.push_back(static_cast<char>(c));
            }
            else if (c == ' ' && spaceAsPlus) {
                // 空格特殊处理（查询参数中常用+）
                result.push_back('+');
            }
            else {
                // 其他字符转换为 %HH 格式
                auto hex = hex_encode(c);
                result.push_back('%');
                result.push_back(static_cast<char>(hex[0]));
                result.push_back(static_cast<char>(hex[1]));
            }
        }

        return result;
    }

    /**
     * @brief URL 解码（类似 Qt 的 fromPercentEncoding）
     * @param input 要解码的字符串
     * @return 解码后的原始字符串
     *
     * 解码规则：
     * 1. %HH 格式转换为对应字节
     * 2. '+' 转换为空格
     * 3. 其他字符保留不变
     */
    inline std::string url_decode(std::string_view input) {
        std::string result;
        result.reserve(input.size()); // 解码后长度不会超过输入

        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];

            if (c == '+') {
                // 加号解码为空格
                result.push_back(' ');
            }
            else if (c == '%' && i + 2 < input.size()) {
                // 处理 %HH 格式
                try {
                    unsigned char byte = hex_decode(
                        static_cast<unsigned char>(input[i + 1]),
                        static_cast<unsigned char>(input[i + 2])
                    );
                    result.push_back(static_cast<char>(byte));
                    i += 2; // 跳过已处理的两位十六进制
                }
                catch (const std::invalid_argument&) {
                    // 无效的十六进制字符，保留 % 原样输出
                    result.push_back('%');
                }
            }
            else {
                // 其他字符直接保留
                result.push_back(c);
            }
        }

        return result;
    }

    /**
     * @brief 高效解析 URI 查询参数
     * @param uri 
     * @param url 如果uri包含"?"，则url为uri"?"的前部分，url = uri
     * @return 解析后的查询参数键值对
     */
    inline std::unordered_map<std::string, std::string> parse_query_params(std::string_view uri, std::string& url) {
        std::unordered_map<std::string, std::string> params;

        // 1. 定位查询字符串起始位置
        const size_t query_start = uri.find('?');
        if (query_start == std::string_view::npos) {
            url = uri;        // 无查询参数，返回原始 URI
            return params; // 没有查询参数
        }

        // 2. 提取查询字符串部分（忽略片段标识符）
        url = uri.substr(0, query_start);
        std::string_view query_str = uri.substr(query_start + 1);
        if (const size_t fragment_pos = query_str.find('#');
            fragment_pos != std::string_view::npos) {
            query_str = query_str.substr(0, fragment_pos);
        }

        // 3. 预分配内存（基于&字符数量）
        const size_t approx_param_count =
            std::count(query_str.begin(), query_str.end(), '&') + 1;
        params.reserve(approx_param_count);

        // 4. 高效状态机解析
        while (!query_str.empty()) {
            // 找到下一个键值对边界
            const size_t pair_end = query_str.find('&');
            std::string_view pair = query_str.substr(0, pair_end);

            // 分割键和值
            const size_t eq_pos = pair.find('=');
            std::string_view key = pair.substr(0, eq_pos);
            std::string_view value = (eq_pos == std::string_view::npos)
                ? std::string_view{}
            : pair.substr(eq_pos + 1);

            // 解码并存储（移动语义避免拷贝）
            params.try_emplace(
                url_decode(key),
                url_decode(value)
            );

            // 移动到下一对参数
            query_str = (pair_end == std::string_view::npos)
                ? std::string_view{}
            : query_str.substr(pair_end + 1);
        }

        return params;
    }

    // 静态方法：将时间点转换为格式化字符串
    inline std::string format_time_point(const std::chrono::system_clock::time_point& timePoint) {
        auto time = std::chrono::system_clock::to_time_t(timePoint);
        std::stringstream ss;
        // 使用线程安全的方式格式化时间
        std::tm tmBuf{};
#ifdef _WIN32
        localtime_s(&tmBuf, &time);
#else
        localtime_r(&time, &tmBuf);
#endif
        ss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // 获取HTTP日期格式的当前时间
    inline std::string get_gmtime() {
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);

        // 转换为GMT时间
        std::tm gmt_tm;
#ifdef _WIN32
        gmtime_s(&gmt_tm, &time_t_now);
#else
        gmtime_r(&time_t_now, &gmt_tm);
#endif

        // 格式化为HTTP日期格式 (RFC 7231)
        // 例如: "Sun, 06 Nov 1994 08:49:37 GMT"
        std::stringstream ss;
        ss << std::put_time(&gmt_tm, "%a, %d %b %Y %H:%M:%S GMT");
        return ss.str();
    }


    //将主机的无符号短整形数转换成网络字节顺序
    inline ::uint16_t htons(std::uint16_t hs)
    {
        return (ENDIANNESS == 'l') ? Swap16(hs) : hs;
    }

    //将主机的无符号长整形数转换成网络字节顺序
    inline std::uint32_t htonl(std::uint32_t hl)
    {
        return (ENDIANNESS == 'l') ? Swap32(hl) : hl;
    }

    //将一个无符号短整形数从网络字节顺序转换为主机字节顺序
    inline std::uint16_t ntohs(std::uint16_t ns)
    {
        return (ENDIANNESS == 'l') ? Swap16(ns) : ns;
    }

    //将一个无符号长整形数从网络字节顺序转换为主机字节顺序
    inline std::uint32_t ntohl(std::uint32_t nl)
    {
        return (ENDIANNESS == 'l') ? Swap32(nl) : nl;
    }
    inline constexpr std::string local_separator() {
#ifdef _WIN32
        return "\\";
#else
        return "/";
#endif
    }

    template<helper::ConstexprString Name>
    std::string get_env_w() {
#if defined(_WIN32)
        // 名称: UTF-8 → UTF-16 (动态缓冲区)
        const int nameLen = MultiByteToWideChar(CP_UTF8, 0, Name, -1, nullptr, 0);
        if (nameLen == 0) throw std::runtime_error("环境变量名转换失败");
        std::wstring wideName(nameLen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, Name, -1, wideName.data(), nameLen);

        // 获取值: 动态适应长度
        DWORD valueLen = GetEnvironmentVariableW(wideName.data(), nullptr, 0);
        if (valueLen == 0) {
            if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
                throw std::runtime_error("环境变量不存在");
            throw std::runtime_error("获取环境变量失败");
        }

        std::wstring wideValue(valueLen, L'\0');
        valueLen = GetEnvironmentVariableW(wideName.data(), wideValue.data(), valueLen);
        if (valueLen == 0) throw std::runtime_error("获取环境变量失败");

        // 值: UTF-16 → UTF-8 (动态缓冲区)
        const int resultLen = WideCharToMultiByte(CP_UTF8, 0,
            wideValue.data(), -1, nullptr, 0, nullptr, nullptr);
        if (resultLen == 0) throw std::runtime_error("值转换失败");

        std::string result(resultLen, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideValue.data(), -1,
            result.data(), resultLen, nullptr, nullptr);
        return result;
#else
        const char* value = std::getenv(Name);
        if (!value) {
            throw std::runtime_error("环境变量不存在");
        }
        return std::string(value);
#endif
/*
#if defined(_WIN32)
        //_wgetenv(Name);
        wchar_t buffer[MAX_PATH];
        char path[MAX_PATH];
        if (!GetEnvironmentVariable(Name, buffer, MAX_PATH)) {
            throw std::runtime_error("获取环境变量MYSQL_HOME失败");
        }
        size_t convertedChars = 0;  // 存储转换后的字节数
        errno_t err = wcstombs_s(
            &convertedChars,  // 接收转换字节数的指针 [CORRECTED]
            path,             // 目标char缓冲区
            MAX_PATH,         // 目标缓冲区大小
            buffer,           // 源wchar_t缓冲区
            _TRUNCATE         // 自动截断超长内容 [SAFER]
        );

        if (err != 0) {
            throw std::runtime_error("宽字符转换失败");
        }
#else
        const char* path = std::getenv(Name)
#endif
*/
    }
};
#ifdef QT_CORE_LIB
namespace FKUtils {
    // 颜色字符串格式枚举
    enum class ColorStringFormat {
        HexARGB,    // #AARRGGBB格式
        RGBAFunction // rgba(r,g,b,a)格式
    };

    namespace helper {
        inline QString toQString(const QString& str) { return str; }
        inline QString toQString(const char* str) { return QString(str); }
        inline QString toQString(const std::string& str) { return QString::fromStdString(str); }
        inline QString toQString(const QLatin1String& str) { return QString(str); }
        
        template<typename T>
        inline auto toQString(const T& value) ->
            typename std::enable_if_t<std::is_arithmetic_v<T>, QString> {
            return QString::number(value);
        }

        inline QString toHexQString(uint value, int width = 2) {
            // 直接构造16进制字符串避免格式化开销
            constexpr const char* hexDigits = "0123456789ABCDEF";
            QString result(width, Qt::Uninitialized);
            QChar* data = result.data();

            for (int i = width - 1; i >= 0; --i) {
                data[i] = QLatin1Char(hexDigits[value & 0xF]);
                value >>= 4;
            }
            return result;
        }
    };

    template<typename... Args>
    inline QString qconcat(Args&&... args) {
        int totalSize = (0 + ... + helper::toQString(args).size());
        QString result;
        result.reserve(totalSize);
        (result += ... += helper::toQString(std::forward<Args>(args)));
        return result;
    }

    template<typename... Args>
    inline QString qjoin(const QString& separator, Args&&... args) {
        if constexpr (sizeof...(args) == 0) return QString();
        int separatorCount = sizeof...(args) - 1;
        int totalSize = (0 + ... + toQString(args).size()) + separatorCount * separator.size();
        QString result;
        result.reserve(totalSize);
        int index = 0;
        ((result += (index++ > 0 ? separator : QString()) +
            toQString(std::forward<Args>(args))), ...);
        return result;
    }

    // 主转换函数模板
    template<ColorStringFormat Format = ColorStringFormat::HexARGB>
    QString colorToCssString(const QColor& color) {
        if (!color.isValid()) return QLatin1String("invalid");

        using namespace helper;

        if constexpr (Format == ColorStringFormat::HexARGB) {
            // 使用qconcat高效拼接
            return qconcat(QLatin1String("#"),
                toHexQString(color.alpha()),
                toHexQString(color.red()),
                toHexQString(color.green()),
                toHexQString(color.blue()));
        }
        else {
            // 使用join构建RGBA函数
            return qjoin(QLatin1String(", "),
                qconcat(QLatin1String("rgba("), color.red()),
                color.green(),
                color.blue(),
                qconcat(QString::number(color.alphaF(), 'f', 3),
                    QLatin1String(")")));
        }
    }
};
#endif
#endif // !FK_UTILS_H_
