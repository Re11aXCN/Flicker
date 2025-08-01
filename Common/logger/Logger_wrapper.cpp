#include "Logger.h"

#ifdef LOGGERWRAPPER_EXPORT
#define LOGGERWRAPPER_API __declspec(dllexport)
#else
#define LOGGERWRAPPER_API __declspec(dllimport)
#endif
#define WRAPPER_LOG(level, ...) \
    do { \
        try { \
            auto logger = Logger::getInstance().getLogger(); \
            if (logger) { \
                logger->log(level, __VA_ARGS__); \
            } \
        } catch (...) {} \
    } while (0)

extern "C" {

    /**
     * @brief 初始化日志记录器。
     * @param filename 日志文件名（不含路径和扩展名）。
     * @param policy 日志生成策略 (0: SingleFile, 1: MultiplePerDay, 2: MultiplePerRun)。
     * @param truncate 是否在启动时清空旧日志。
     * @return 如果初始化成功，返回 true。
     */
    LOGGERWRAPPER_API bool Logger_Initialize(const char* fileName, int policy, bool truncate = false, const char* fileDir = nullptr) {
        // 将 C 风格的 int 转换为 C++ 的 enum
        auto _policy = static_cast<Logger::GeneratePolicy>(policy);
        // 调用原始 C++ 单例的方法
        return Logger::getInstance().initialize(std::string(fileName), _policy, truncate, fileDir);
    }

    /**
     * @brief 关闭并刷新所有日志。
     */
    LOGGERWRAPPER_API void Logger_Shutdown() {
        Logger::getInstance().shutdown();
    }

    /**
     * @brief 手动刷新日志缓冲区。
     */
    LOGGERWRAPPER_API void Logger_Flush() {
        Logger::getInstance().flush();
    }

    LOGGERWRAPPER_API void Logger_Info(const char* message) {
        WRAPPER_LOG(spdlog::level::info, message);
    }
    LOGGERWRAPPER_API void Logger_Warn(const char* message) {
        WRAPPER_LOG(spdlog::level::warn, message);
    }
    LOGGERWRAPPER_API void Logger_Error(const char* message) {
        WRAPPER_LOG(spdlog::level::err, message);
    }
    LOGGERWRAPPER_API void Logger_Trace(const char* message) {
        WRAPPER_LOG(spdlog::level::trace, message);
    }
    LOGGERWRAPPER_API void Logger_Critical(const char* message) {
        WRAPPER_LOG(spdlog::level::critical, message);
    } 
    LOGGERWRAPPER_API void Logger_Debug(const char* message) {
        WRAPPER_LOG(spdlog::level::debug, message);
    }
}
