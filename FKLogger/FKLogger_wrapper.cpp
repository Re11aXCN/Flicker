#include "FKLogger.h" // 引入你原始的头文件
#include <string>

#ifdef FKLOGGERWRAPPER_EXPORT
#define FKLOGGERWRAPPER_API __declspec(dllexport)
#else
#define FKLOGGERWRAPPER_API __declspec(dllimport)
#endif

extern "C" {

    /**
     * @brief 初始化日志记录器。
     * @param filename 日志文件名（不含路径和扩展名）。
     * @param policy 日志生成策略 (0: SingleFile, 1: MultiplePerDay, 2: MultiplePerRun)。
     * @param truncate 是否在启动时清空旧日志。
     * @return 如果初始化成功，返回 true。
     */
    FKLOGGERWRAPPER_API bool FKLogger_Initialize(const char* filename, int policy, bool truncate) {
        // 将 C 风格的 int 转换为 C++ 的 enum
        auto fk_policy = static_cast<FKLogger::GeneratePolicy>(policy);
        // 调用原始 C++ 单例的方法
        return FKLogger::getInstance().initialize(std::string(filename), fk_policy, truncate);
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

    // 注意：你不能直接调用 C++ 的宏 (FK_CLIENT_INFO)。
    // 我们需要创建函数来间接实现日志记录。

    /**
     * @brief 记录一条 Info 级别的日志。
     * @param message 要记录的日志消息。
     */
    FKLOGGERWRAPPER_API void FKLogger_Info(const char* message) {
        // 我们在这里调用原始的宏，{} 是 spdlog 的格式化占位符
        FK_CLIENT_INFO("{}", message);
    }

    /**
     * @brief 记录一条 Warn 级别的日志。
     * @param message 要记录的日志消息。
     */
    FKLOGGERWRAPPER_API void FKLogger_Warn(const char* message) {
        FK_CLIENT_WARN("{}", message);
    }

    /**
     * @brief 记录一条 Error 级别的日志。
     * @param message 要记录的日志消息。
     */
    FKLOGGERWRAPPER_API void FKLogger_Error(const char* message) {
        FK_CLIENT_ERROR("{}", message);
    }

    /**
     * @brief 记录一条 Debug 级别的日志。
     * @param message 要记录的日志消息。
     */
    FKLOGGERWRAPPER_API void FKLogger_Debug(const char* message) {
        FK_CLIENT_DEBUG("{}", message);
    }
}
