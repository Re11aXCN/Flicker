﻿/*************************************************************************************
 *
 * @ Filename     : FKRedisConnectionPool.h
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
#ifndef FK_REDIS_CONNECTION_POOL_H_
#define FK_REDIS_CONNECTION_POOL_H_

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>

#include <functional>
#include <sw/redis++/redis++.h>
#include "FKMacro.h"
#include "FKLogger.h"
#include "Source/FKStructConfig.h"

// Redis连接池类，使用单例模式
class FKRedisConnectionPool {
    SINGLETON_CREATE_SHARED_H(FKRedisConnectionPool)

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
            FK_SERVER_ERROR(std::format("获取Redis连接失败: {}", e.what()));
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
                FK_SERVER_INFO(std::format("Redis操作耗时较长: {}ms", duration.count()));
            }
            
            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            FK_SERVER_ERROR(std::format("Redis执行异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        } catch (...) {
            FK_SERVER_ERROR("Redis执行未知异常");
            throw; // 重新抛出未知异常，让调用者处理
        }
    }

    // 获取连接池状态信息
    std::string getStatus() const;

private:
    FKRedisConnectionPool();
    ~FKRedisConnectionPool();
    void _initialize(const FKRedisConfig& config);
    std::unique_ptr<sw::redis::Redis> _createConnection();

    // 连接池配置
    FKRedisConfig _pConfig;

    // 连接池
    std::queue<std::unique_ptr<sw::redis::Redis>> _pConnectionPool;

    // 互斥锁，保证线程安全
    mutable std::mutex _pMutex;

    // 连接池状态
    bool _pIsInitialized{false};
};

#endif // !FK_REDIS_CONNECTION_POOL_H_