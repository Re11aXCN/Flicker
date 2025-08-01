#ifndef LOGGER_H_
#define LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#ifdef LOGGER_EXPORT
#define LOGGER_API __declspec(dllexport)
#else
#define LOGGER_API __declspec(dllimport)
#endif
namespace fs = std::filesystem;
class LOGGER_API Logger {
public:
    enum GeneratePolicy : std::uint8_t {
        SingleFile,       // 单个日志文件
        MultiplePerDay,   // 按日期生成文件

        MultiplePerRun,   // 每次运行生成新文件，不能追加
    };

    static Logger& getInstance();
    std::shared_ptr<spdlog::logger> getLogger() const;

    [[nodiscard("Logs can only be used after initialization is complete!")]] 
    bool initialize(const std::string& fileName, GeneratePolicy generatePolicy, bool truncate = false
        , const std::string& fileDir = "");

    void shutdown();
    void flush();
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string _createLogFlie(const std::string& fileDir, const std::string& fileName);
    fs::path _getExecutablePath();

    GeneratePolicy _generatePolicy{ SingleFile };

    std::mutex _mutex;
    std::shared_ptr<spdlog::logger> _logger;
};

//spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }
//, " (File: " __FILE__ ", Function: " __FUNCSIG__ ", Line: " + __LINE__ + ")"
#ifdef _DEBUG
#define _LOG(level, ...) \
    do { \
        try { \
            auto logger = Logger::getInstance().getLogger(); \
            if (logger) { \
                logger->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__); \
            } \
        } catch (...) {} \
    } while (0)
#elif defined(_RELWITHDEBINFO)
#define _LOG(level, ...) \
    do { \
        try { \
            auto logger = Logger::getInstance().getLogger(); \
            if (logger) { \
                logger->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__); \
            } \
        } catch (...) {} \
    } while (0)
#elif defined(_RELEASE)
#define _LOG(level, ...)
#endif

#define LOGGER_TRACE(...)    _LOG(spdlog::level::trace, __VA_ARGS__)
#define LOGGER_DEBUG(...)    _LOG(spdlog::level::debug, __VA_ARGS__)
#define LOGGER_INFO(...)     _LOG(spdlog::level::info, __VA_ARGS__)
#define LOGGER_WARN(...)     _LOG(spdlog::level::warn, __VA_ARGS__)
#define LOGGER_ERROR(...)    _LOG(spdlog::level::err, __VA_ARGS__)
#define LOGGER_CRITICAL(...) _LOG(spdlog::level::critical, __VA_ARGS__)

#endif // LOGGER_H_
