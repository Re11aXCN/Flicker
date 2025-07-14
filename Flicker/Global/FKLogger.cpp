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
FKLogger::FKLogger()
{

}

FKLogger::~FKLogger()
{
    if(_logger) _logger->flush();
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

bool FKLogger::initialize(const std::string& filename, GeneratePolicy generatePolicy, bool truncate/* = false*/)
{
    try {
        std::lock_guard<std::mutex> lock(_mutex);

        _filename = filename;
        _generatePolicy = generatePolicy;

        spdlog::init_thread_pool(8192, 1);


        std::string actualFilename = _createLogFlie();

        // 创建控制台和文件接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(actualFilename, truncate);
        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };

        _logger = std::make_shared<spdlog::async_logger>(actualFilename, sinks.begin(), sinks.end(),
            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        spdlog::register_logger(_logger);
        

        // 完整调试格式（含线程、文件、函数、行号）
        _logger->set_pattern("%^[%Y-%m-%d %T.%e] [tid: %t] {File: %s Func: %! Line: %#} \r\n\t[%l]: %v%$");

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

void FKLogger::shutdow()
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

std::string FKLogger::_createLogFlie()
{    
    // 创建logs目录（如果不存在）
    fs::path appParentDir = _getExecutablePath().parent_path();
    fs::path logsDir = appParentDir / "logs";
    fs::create_directories(logsDir);

    std::string actualFilename;
    switch (_generatePolicy)
    {
    case SingleFile: {
        actualFilename = FKUtils::concat(logsDir.string(), FKUtils::local_separator(), _filename, ".log");
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

        actualFilename = FKUtils::concat(logsDir.string(), FKUtils::local_separator(), _filename, " - [", dateStr.str(), "].log");
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
        
        actualFilename = FKUtils::concat(logsDir.string(), FKUtils::local_separator(), _filename, " - [", dateStr.str(), "].log");
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
