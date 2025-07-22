#include "FKLogger.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

#include "FKUtils.h"
template<typename Mutex>
class html_format_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit html_format_sink(const spdlog::filename_t& filename,
        bool truncate = false,
        const spdlog::file_event_handlers& event_handlers = {})
        : file_helper_{ event_handlers }
    {
        //  - 如果文件不存在，创建后大小为0，写入header。
        //  -如果文件存在且truncate = true，那么文件被清空，大小为0，写入header。
        //  - 如果文件存在且truncate = false（追加模式），且文件大小为0（可能是之前创建但没写入内容），那么写入
        bool file_empty = true;
        try {
            if (fs::exists(filename)) {
                file_empty = (fs::file_size(filename) == 0);
            }
        }
        catch (...) {
            // 文件访问异常时视为空文件
            file_empty = true;
        }

        file_helper_.open(filename, truncate);

        if (file_empty || truncate) {
            _write_header();
        }
    }

    ~html_format_sink()
    {
        // 不再闭合html 浏览器对不完整 HTML 有容错机制，能正常渲染已写入内容
        //_write_footer();
    }

    const spdlog::filename_t& filename() const {
        return file_helper_.filename();
    }

    void truncate() {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        file_helper_.reopen(true);
    }
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        // 格式化消息
        spdlog::memory_buf_t msg_formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, msg_formatted);

        // 根据日志级别设置颜色
        const char* prefix = _level_to_color(msg.level);
        constexpr const char* suffix = "</font><br>\n";

        spdlog::memory_buf_t font_prefix, font_suffix;
        font_prefix.append(prefix, prefix + std::strlen(prefix));
        font_suffix.append(suffix, suffix + std::strlen(suffix));

        // 写入带颜色的日志消息
        file_helper_.write(font_prefix);
        file_helper_.write(msg_formatted);
        file_helper_.write(font_suffix);
    }

    void flush_() override
    {
        file_helper_.flush();
    }

private:
    void _write_header()
    {
        // HTML头部，使用UTF-8编码，设置微软雅黑字体
        static constexpr const char* header = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Log Output</title>
<style>
body {
    background-color: #282C34;
    color: #ABB2BF;
    font-family: "Microsoft YaHei", "微软雅黑", sans-serif;
    font-size: 14px;
    line-height: 1.5;
    margin: 10px;
    white-space: pre-wrap;
}
</style>
</head>
<body>
)";

        spdlog::memory_buf_t html_header;
        html_header.append(header, header + std::strlen(header));
        file_helper_.write(html_header);
    }

    void _write_footer()
    {
        // HTML尾部
        static constexpr const char* footer = R"(</body></html>)";

        spdlog::memory_buf_t html_footer;
        html_footer.append(footer, footer + std::strlen(footer));
        file_helper_.write(html_footer);
    }

    static const char* _level_to_color(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace: return R"(<font color="#DCDFE4">)";
        case spdlog::level::debug: return R"(<font color="#56B6C2">)";
        case spdlog::level::info: return R"(<font color="#98C379">)";
        case spdlog::level::warn: return R"(<font color="#E5C07B">)";
        case spdlog::level::err: return R"(<font color="#E06C75">)";
        case spdlog::level::critical: return R"(<font color="#DCDFE4" style="background-color:#E06C75;">)";
        case spdlog::level::off:
        case spdlog::level::n_levels: break;
        }
        return "";
    }

    // 文件辅助类，用于文件操作
    spdlog::details::file_helper file_helper_;
};

using html_format_sink_mt = html_format_sink<std::mutex>;
using html_format_sink_st = html_format_sink<spdlog::details::null_mutex>;


FKLogger::FKLogger()
{

}

FKLogger::~FKLogger()
{
}

FKLogger& FKLogger::getInstance()
{
    static FKLogger instance;
    return instance;
}

std::shared_ptr<spdlog::logger> FKLogger::getLogger() const
{
    return _logger;
}

bool FKLogger::initialize(const std::string& fileName, GeneratePolicy generatePolicy, bool truncate/* = false*/
    , const std::string& fileDir/* = ""*/)
{
    try {
        std::lock_guard<std::mutex> lock(_mutex);

        _generatePolicy = generatePolicy;

        spdlog::init_thread_pool(8192, 1);

        std::string actualFilename;
        // 创建logs目录（如果不存在）
        if (fileDir.empty()) {
            fs::path appParentDir = _getExecutablePath().parent_path();
            fs::path logsDir = appParentDir / "logs";
            fs::create_directories(logsDir);
            actualFilename = _createLogFlie(logsDir.string(), fileName);
        }
        else {
            actualFilename = _createLogFlie(fileDir, fileName);
        }

        // 创建控制台和HTML文件接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_format_sink = std::make_shared<html_format_sink_mt>(actualFilename, truncate);
        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_format_sink };

        _logger = std::make_shared<spdlog::async_logger>(actualFilename, sinks.begin(), sinks.end(),
            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        spdlog::register_logger(_logger);


        // 完整调试格式（含线程、文件、函数、行号）
#ifdef _RELEASE
        _logger->set_pattern("%^[%Y-%m-%d %T.%e] [tid: %t] [%l]: %v%$");
#else
        _logger->set_pattern("%^[%Y-%m-%d %T.%e] [tid: %t] [%l]: %v {File: %s Func: %! Line: %#}%$");
#endif

#ifdef _DEBUG
        _logger->set_level(spdlog::level::debug);
#else
        _logger->set_level(spdlog::level::info);
#endif

        // 设置刷新间隔（秒）
        _logger->flush_on(spdlog::level::err);
        spdlog::flush_every(std::chrono::seconds(3));
        return true;
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

void FKLogger::shutdown()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_logger) {
        _logger->info("FKLogger shutdown!!!");
        _logger->flush();
        spdlog::shutdown();
        _logger.reset();
    }
}

void FKLogger::flush()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_logger) {
        _logger->flush();
    }
}

std::string FKLogger::_createLogFlie(const std::string& fileDir, const std::string& fileName)
{
    std::string actualFilename;
    switch (_generatePolicy)
    {
    case SingleFile: {
        actualFilename = FKUtils::concat(fileDir, FKUtils::local_separator(), fileName, ".log.html");
        break;
    }
    case MultiplePerDay: {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_now);
#endif
        std::stringstream dateStr;
        dateStr << std::put_time(&tm_now, "%Y-%m-%d");

        actualFilename = FKUtils::concat(fileDir, FKUtils::local_separator(), fileName, " - [", dateStr.str(), "].log.html");
        break;
    }
    case MultiplePerRun: {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_now);
#endif
        std::stringstream dateStr;
        dateStr << std::put_time(&tm_now, "#%Y-%m-%d  #%H-%M-%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();

        actualFilename = FKUtils::concat(fileDir, FKUtils::local_separator(), fileName, " - [", dateStr.str(), "].log.html");
        break;
    }
    default: throw std::runtime_error("There is no policy specified for generating log files!");
    }
    return actualFilename;
}

fs::path FKLogger::_getExecutablePath()
{
    // Windows 实现
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return buffer;

    // macOS 实现
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        throw std::runtime_error("Path buffer too small");
    }
    return fs::canonical(buffer); // 自动解析符号链接

    // Linux 实现
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        throw std::runtime_error("Failed to read /proc/self/exe");
    }
    buffer[len] = '\0';
    return buffer;
#endif
}
