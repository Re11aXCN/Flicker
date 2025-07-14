#ifndef FK_LOGGER_H_
#define FK_LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
namespace fs = std::filesystem;
class FKLogger {
public:
    enum GeneratePolicy : std::uint8_t {
        SingleFile,       // 单个日志文件
        MultiplePerDay,   // 按日期生成文件

        MultiplePerRun,   // 每次运行生成新文件，不能追加
    };

    static FKLogger& getInstance();
    std::shared_ptr<spdlog::logger> getLogger() const;

    [[nodiscard("Logs can only be used after initialization is complete!")]] 
    bool initialize(const std::string& filename, GeneratePolicy generatePolicy, bool truncate = false);

    void shutdow();
    void flush();
private:
    FKLogger();
    ~FKLogger();
    FKLogger(const FKLogger&) = delete;
    FKLogger& operator=(const FKLogger&) = delete;

    std::string _createLogFlie();
    fs::path _getExecutablePath();

    GeneratePolicy _generatePolicy{ SingleFile };
    std::string _filename;

    std::mutex _mutex;
    std::shared_ptr<spdlog::logger> _logger;
};

//spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }
//, " (File: " __FILE__ ", Function: " __FUNCSIG__ ", Line: " + __LINE__ + ")"
#ifdef _DEBUG
#define FK_LOG(level, ...) \
    do { \
        try { \
            auto logger = FKLogger::getInstance().getLogger(); \
            if (logger) { \
                logger->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__); \
            } \
        } catch (...) {} \
    } while (0)
#elif defined(_RELWITHDEBINFO)
#define FK_LOG(level, ...) \
    do { \
        try { \
            auto logger = FKLogger::getInstance().getLogger(); \
            if (logger) { \
                logger->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__); \
            } \
        } catch (...) {} \
    } while (0)
#elif defined(_RELEASE)
#define FK_LOG(level, ...)
#endif

#define FK_CLIENT_TRACE(...)    FK_LOG(spdlog::level::trace, __VA_ARGS__)
#define FK_CLIENT_INFO(...)     FK_LOG(spdlog::level::info, __VA_ARGS__)
#define FK_CLIENT_WARN(...)     FK_LOG(spdlog::level::warn, __VA_ARGS__)
#define FK_CLIENT_ERROR(...)    FK_LOG(spdlog::level::err, __VA_ARGS__)
#define FK_CLIENT_CRITICAL(...) FK_LOG(spdlog::level::critical, __VA_ARGS__)

#define FK_SERVER_TRACE(...)    FK_LOG(spdlog::level::trace, __VA_ARGS__)
#define FK_SERVER_INFO(...)     FK_LOG(spdlog::level::info, __VA_ARGS__)
#define FK_SERVER_WARN(...)     FK_LOG(spdlog::level::warn, __VA_ARGS__)
#define FK_SERVER_ERROR(...)    FK_LOG(spdlog::level::err, __VA_ARGS__)
#define FK_SERVER_CRITICAL(...) FK_LOG(spdlog::level::critical, __VA_ARGS__)

#endif // FK_LOGGER_H_
