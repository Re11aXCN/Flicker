/*************************************************************************************
 *
 * @ Filename	 : FKMySQLConnectionPool.h
 * @ Description : MySQL连接池，使用纯C API
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
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
#include <print>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "FKMacro.h"
#include "Source/FKStructConfig.h"
#include "FKMySQLConnection.h"
// FKMySQLConnection类已移至单独的文件

// MySQL连接池类，使用单例模式
class FKMySQLConnectionPool {
    SINGLETON_CREATE_H(FKMySQLConnectionPool)
		
public:
    // 获取连接
    std::shared_ptr<FKMySQLConnection> getConnection();
    
    // 释放连接
    void releaseConnection(std::shared_ptr<FKMySQLConnection> conn);
    
    // 关闭连接池
    void shutdown();

    // 使用连接执行操作的模板方法
    template<typename Func>
    auto executeWithConnection(Func&& func) {
        // 使用shared_ptr确保连接在任何情况下都能被正确释放
        auto conn = getConnection();
        if (!conn || !conn->getMysql()) {
            throw std::runtime_error("无法获取有效的数据库连接");
        }
        
        try {
            // 更新最后活动时间
            conn->updateActiveTime();
            // 执行操作并捕获结果
            auto result = func(conn->getMysql());
            // 操作成功，释放连接
            releaseConnection(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            std::println("MySQL执行异常: {}", e.what());
            throw;
        } catch (...) {
            // 捕获所有其他类型的异常
            std::println("执行数据库操作时发生未知异常");
            throw;
        }
    }

    // 执行事务操作的模板方法
    template<typename Func>
    auto executeTransaction(Func&& func) {
        // 使用shared_ptr确保连接在任何情况下都能被正确释放
        auto conn = getConnection();
        if (!conn || !conn->getMysql()) {
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }
        
        try {
            // 更新最后活动时间
            conn->updateActiveTime();
            auto mysql = conn->getMysql();
            
            // 开始事务
            conn->startTransaction();
            
            try {
                // 执行事务操作
                auto result = func(mysql);
                
                // 提交事务
                conn->commit();
                
                releaseConnection(std::move(conn));
                return result;
            } catch (const std::exception& e) {
                // 事务操作异常，回滚事务
                std::println("事务执行异常，执行回滚: {}", e.what());
                conn->rollback();
                releaseConnection(std::move(conn));
                throw;
            } catch (...) {
                // 捕获所有其他类型的异常
                std::println("执行事务操作时发生未知异常，执行回滚");
                conn->rollback();
                releaseConnection(std::move(conn));
                throw;
            }
        } catch (const std::exception& e) {
            // 连接可能已损坏，不放回池中
            std::println("MySQL事务异常: {}", e.what());
            throw;
        } catch (...) {
            // 捕获所有其他类型的异常
            std::println("MySQL事务发生未知异常");
            throw;
        }
    }

    // 获取连接池状态信息
    std::string getStatus() const;

private:
    FKMySQLConnectionPool();
    ~FKMySQLConnectionPool();
    
    // 初始化连接池
    void _initialize(const FKMySQLConfig& config);
    
    // 创建新连接
    std::shared_ptr<FKMySQLConnection> _createConnection();
    
    // 监控线程函数，定期检查并关闭超时连接
    void _monitorThreadFunc();

    // 连接池配置
    FKMySQLConfig _pConfig;

    // 连接池
    std::queue<std::shared_ptr<FKMySQLConnection>> _pConnectionPool;

    // 互斥锁，保证线程安全
    mutable std::mutex _pMutex;
    
    // 条件变量，用于监控线程的等待和通知
    std::condition_variable _pCondVar;

    // 连接池状态
    std::atomic<bool> _pIsInitialized{false};
    std::atomic<bool> _pIsShutdown{false};
    
    // 监控线程
    std::thread _pMonitorThread;
};

#endif // !FK_MYSQL_CONNECTION_POOL_H_