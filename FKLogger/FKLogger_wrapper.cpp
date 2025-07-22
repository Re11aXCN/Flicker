#include "FKLogger.h"

#ifdef FKLOGGERWRAPPER_EXPORT
#define FKLOGGERWRAPPER_API __declspec(dllexport)
#else
#define FKLOGGERWRAPPER_API __declspec(dllimport)
#endif
#define FKWRAPPER_LOG(level, ...) \
    do { \
        try { \
            auto logger = FKLogger::getInstance().getLogger(); \
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
    FKLOGGERWRAPPER_API bool FKLogger_Initialize(const char* fileName, int policy, bool truncate = false, const char* fileDir = nullptr) {
        // 将 C 风格的 int 转换为 C++ 的 enum
        auto fk_policy = static_cast<FKLogger::GeneratePolicy>(policy);
        // 调用原始 C++ 单例的方法
        return FKLogger::getInstance().initialize(std::string(fileName), fk_policy, truncate, fileDir);
    }

    /**
     * @brief 关闭并刷新所有日志。
     */
    FKLOGGERWRAPPER_API void FKLogger_Shutdown() {
        FKLogger::getInstance().shutdown();
    }

    /**
     * @brief 手动刷新日志缓冲区。
     */
    FKLOGGERWRAPPER_API void FKLogger_Flush() {
        FKLogger::getInstance().flush();
    }

    FKLOGGERWRAPPER_API void FKLogger_Info(const char* message) {
        FKWRAPPER_LOG(spdlog::level::info, message);
    }
    FKLOGGERWRAPPER_API void FKLogger_Warn(const char* message) {
        FKWRAPPER_LOG(spdlog::level::warn, message);
    }
    FKLOGGERWRAPPER_API void FKLogger_Error(const char* message) {
        FKWRAPPER_LOG(spdlog::level::err, message);
    }
    FKLOGGERWRAPPER_API void FKLogger_Trace(const char* message) {
        FKWRAPPER_LOG(spdlog::level::trace, message);
    }
    FKLOGGERWRAPPER_API void FKLogger_Critical(const char* message) {
        FKWRAPPER_LOG(spdlog::level::critical, message);
    } 
    FKLOGGERWRAPPER_API void FKLogger_Debug(const char* message) {
        FKWRAPPER_LOG(spdlog::level::debug, message);
    }
}
