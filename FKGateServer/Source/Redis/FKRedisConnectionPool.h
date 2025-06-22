/*************************************************************************************
 *
 * @ Filename	 : FKRedisConnectionPool.h
 * @ Description : Redis连接池，管理Redis连接的创建、获取和释放
 *
 * @ Version	 : V1.0
 * @ Author	 : Re11a
 * @ Date Created: 2025/7/10
*************************************************************************************/
#ifndef FK_REDIS_CONNECTION_POOL_H_
#define FK_REDIS_CONNECTION_POOL_H_

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <print>
#include <functional>
#include <sw/redis++/redis++.h>
#include "FKMacro.h"
#include "../FKConfigManager.h"

// Redis连接池类，使用单例模式
class FKRedisConnectionPool {
    SINGLETON_CREATE_SHARED_H(FKRedisConnectionPool)

public:
	std::unique_ptr<sw::redis::Redis> getConnection();
	void releaseConnection(std::unique_ptr<sw::redis::Redis> conn);
	void shutdown();

    // 使用连接执行操作的模板方法
    template<typename Func>
    auto executeWithConnection(Func&& func) {
        auto conn = getConnection();
        try {
            auto result = func(conn.get());
            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            std::println("Redis执行异常: {}", e.what());
            throw;
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