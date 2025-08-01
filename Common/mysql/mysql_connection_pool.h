/*************************************************************************************
 *
 * @ Filename     : MySQLConnectionPool.h
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
#ifndef MYSQL_CONNECTION_POOL_H_
#define MYSQL_CONNECTION_POOL_H_

#include <string>
#include <format>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "Common/global/macro.h"
#include "Common/logger/logger_defend.h"
#include "Common/config/struct_config.h"
#include "Common/mysql/mysql_connection.h"

class MySQLConnectionPool {
    SINGLETON_CREATE_H(MySQLConnectionPool)
        
public:
    std::shared_ptr<MySQLConnection> getConnection();
    void releaseConnection(std::shared_ptr<MySQLConnection> conn);
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
            LOGGER_ERROR(std::format("MySQL执行异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        } catch (...) {
            LOGGER_ERROR("执行数据库操作时发生未知异常");
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
            LOGGER_ERROR(std::format("MySQL事务异常: {}", e.what()));
            conn->rollback();
            releaseConnection(std::move(conn));
            throw;
        } catch (...) {
            LOGGER_ERROR("执行事务操作时发生未知异常，执行回滚");
            conn->rollback();
            releaseConnection(std::move(conn));
            throw;
        }
    }
    std::string getStatus() const;

private:
    MySQLConnectionPool();
    ~MySQLConnectionPool();
    
    void _initialize(const MySQLConfig& config);
    std::shared_ptr<MySQLConnection> _createConnection();
    
    // 监控线程函数，定期检查并关闭超时连接
    void _monitorThreadFunc();

    MySQLConfig _pConfig;
    std::queue<std::shared_ptr<MySQLConnection>> _pConnectionPool;
    mutable std::mutex _pMutex;
    std::condition_variable _pCondVar;
    std::atomic<bool> _pIsInitialized{false};
    std::atomic<bool> _pIsShutdown{false};
    std::thread _pMonitorThread;
};

#endif // !MYSQL_CONNECTION_POOL_H_