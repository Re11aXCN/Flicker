/*************************************************************************************
 *
 * @ Filename     : FKMySQLConnectionPool.h
 * @ Description : MySQL连接池，使用纯C API
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
#ifndef FK_MYSQL_CONNECTION_POOL_H_
#define FK_MYSQL_CONNECTION_POOL_H_

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "FKMacro.h"
#include "FKLogger.h"
#include "Source/FKStructConfig.h"
#include "FKMySQLConnection.h"
class FKMySQLConnectionPool {
    SINGLETON_CREATE_H(FKMySQLConnectionPool)
        
public:
    std::shared_ptr<FKMySQLConnection> getConnection();
    void releaseConnection(std::shared_ptr<FKMySQLConnection> conn);
    void shutdown();

    template<typename Func>
    auto executeWithConnection(Func&& func) -> decltype(func(std::declval<MYSQL*>())) {
        auto conn = getConnection();
        if (!conn) {
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }
        auto mysql = conn->getMysql();
        if (!mysql) {
            releaseConnection(std::move(conn));
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }

        try {
            // 更新最后活动时间
            conn->updateActiveTime();
            // 执行操作并捕获结果
            auto result = func(mysql);
            // 操作成功，释放连接
            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            FK_SERVER_ERROR(std::format("MySQL执行异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        } catch (...) {
            FK_SERVER_ERROR("执行数据库操作时发生未知异常");
            throw;
        }
    }

    template<typename Func>
    auto executeTransaction(Func&& func) -> decltype(func(std::declval<MYSQL*>())) {
        auto conn = getConnection();
        if (!conn) {
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }
        auto mysql = conn->getMysql();
        if (!mysql) {
            releaseConnection(std::move(conn));
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }
        try {
            conn->updateActiveTime();
            if (!conn->startTransaction()) {
                releaseConnection(std::move(conn));
                throw std::runtime_error("无法开始事务");
            }
            
            // 执行事务操作
            auto result = func(mysql);
            if (!conn->commit()) {
                conn->rollback();
                releaseConnection(std::move(conn));
                throw std::runtime_error("提交事务失败");
            }

            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("MySQL事务异常: {}", e.what()));
            conn->rollback();
            releaseConnection(std::move(conn));
            throw;
        } catch (...) {
            FK_SERVER_ERROR("执行事务操作时发生未知异常，执行回滚");
            conn->rollback();
            releaseConnection(std::move(conn));
            throw;
        }
    }
    std::string getStatus() const;

private:
    FKMySQLConnectionPool();
    ~FKMySQLConnectionPool();
    
    void _initialize(const FKMySQLConfig& config);
    std::shared_ptr<FKMySQLConnection> _createConnection();
    
    // 监控线程函数，定期检查并关闭超时连接
    void _monitorThreadFunc();

    FKMySQLConfig _pConfig;
    std::queue<std::shared_ptr<FKMySQLConnection>> _pConnectionPool;
    mutable std::mutex _pMutex;
    std::condition_variable _pCondVar;
    std::atomic<bool> _pIsInitialized{false};
    std::atomic<bool> _pIsShutdown{false};
    std::thread _pMonitorThread;
};

#endif // !FK_MYSQL_CONNECTION_POOL_H_