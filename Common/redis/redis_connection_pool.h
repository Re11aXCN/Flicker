/*************************************************************************************
 *
 * @ Filename     : RedisConnectionPool.h
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/22
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef REDIS_CONNECTION_POOL_H_
#define REDIS_CONNECTION_POOL_H_

#include <string>
#include <queue>
#include <chrono>
#include <functional>
#include <mutex>
#include <memory>

#pragma warning(push)
#pragma warning(disable:4200)
#include <sw/redis++/redis++.h>
#pragma warning(pop)

#include "Common/global/macro.h"
#include "Common/config/struct_config.h"
#include "Common/logger/logger_defend.h"
// Redis连接池类，使用单例模式
class RedisConnectionPool {
    SINGLETON_CREATE_SHARED_H(RedisConnectionPool)

public:
    std::unique_ptr<sw::redis::Redis> getConnection();
    void releaseConnection(std::unique_ptr<sw::redis::Redis> conn);
    void shutdown();

    // 使用连接执行操作的模板方法
    template<typename Func>
    auto executeWithConnection(Func&& func) -> decltype(func(std::declval<sw::redis::Redis*>())) {
        std::unique_ptr<sw::redis::Redis> conn;
        try {
            conn = getConnection();
        } catch (const std::exception& e) {
            LOGGER_ERROR(std::format("获取Redis连接失败: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        }
        
        if (!conn) {
            throw std::runtime_error("获取到无效的Redis连接");
        }
        
        try {
            auto startTime = std::chrono::steady_clock::now();
            auto result = func(conn.get());
            auto endTime = std::chrono::steady_clock::now();
            
            // 计算执行时间
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            if (duration.count() > 50) { // 只记录耗时超过50ms的调用
                LOGGER_INFO(std::format("Redis操作耗时较长: {}ms", duration.count()));
            }
            
            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            LOGGER_ERROR(std::format("Redis执行异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        } catch (...) {
            LOGGER_ERROR("Redis执行未知异常");
            throw; // 重新抛出未知异常，让调用者处理
        }
    }

    // 获取连接池状态信息
    std::string getStatus() const;

private:
    RedisConnectionPool();
    ~RedisConnectionPool();
    void _initialize(const RedisConfig& config);
    std::unique_ptr<sw::redis::Redis> _createConnection();

    // 连接池配置
    RedisConfig _pConfig;

    // 连接池
    std::queue<std::unique_ptr<sw::redis::Redis>> _pConnectionPool;

    // 互斥锁，保证线程安全
    mutable std::mutex _pMutex;

    // 连接池状态
    bool _pIsInitialized{false};
};

#endif // !REDIS_CONNECTION_POOL_H_