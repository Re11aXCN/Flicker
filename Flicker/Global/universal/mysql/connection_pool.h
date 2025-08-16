/*************************************************************************************
 *
 * @ Filename     : connection_pool.h
 * @ Description : MySQL连接池，使用纯C API，参考redis++设计
 * 
 * @ Version     : V3.0
 * @ Author      : Re11a
 * @ Date Created: 2025/6/22
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V3.0    Modify Time: 2025/7/10    Modified By: Re11a
 * Modifications: 移除单例模式，重构连接池设计
 * ======================================
*************************************************************************************/
#ifndef UNIVERSAL_MYSQL_CONNECTION_POOL_H_
#define UNIVERSAL_MYSQL_CONNECTION_POOL_H_

#include <string>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <functional>

#include "universal/macros.h"
#include "connection.h"
#include "Library/Logger/logger.h"

namespace universal::mysql {

// 连接池选项和连接的time设置不同 https://zhuanlan.zhihu.com/p/82554484
struct ConnectionPoolOptions {
    // 连接池大小，包括使用中和空闲的连接
    std::size_t size = 5;
    
    // 获取连接的最大等待时间，0表示永久等待
    std::chrono::seconds wait_timeout{0};
    
    // 连接的最大生命周期，0表示永不过期
    std::chrono::seconds connection_lifetime{0};
    
    // 连接的最大空闲时间，0表示永不过期
    std::chrono::seconds connection_idle_time{1800}; // 默认30分钟
    
    // 监控线程检查间隔
    std::chrono::seconds monitor_interval{300}; // 默认5分钟
};

// 连接池类
class ConnectionPool {
public:
    ConnectionPool(ConnectionOptions connection_opts,
        ConnectionPoolOptions pool_opts = ConnectionPoolOptions{});
    
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    ConnectionPool(ConnectionPool&& that) noexcept;
    ConnectionPool& operator=(ConnectionPool&& that) noexcept;
    
    ~ConnectionPool();
    
    // 从连接池获取一个连接
    Connection fetch();
    
    // 获取连接选项
    ConnectionOptions connection_options() const;
    
    // 释放连接回连接池
    void release(Connection connection);
    
    // 创建一个新连接
    Connection create();
    
    // 克隆连接池
    std::shared_ptr<ConnectionPool> clone() const;
    
    // 关闭连接池
    void shutdown();
    
    // 获取连接池状态信息
    std::string get_status() const;
    
    // 使用连接执行操作
    template<typename Func>
    auto execute_with_connection(Func&& func) -> decltype(func(std::declval<MYSQL*>()))
    {
        auto conn = fetch();
        if (conn.broken()) {
            throw std::runtime_error("无法获取有效的数据库连接");
        }
        
        try {
            // 执行操作并捕获结果
            auto result = func(conn.get_mysql());
            conn.update_active_time();
            // 操作成功，释放连接
            release(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            // 连接可能已损坏，标记为无效
            conn.invalidate();
            LOGGER_ERROR(std::format("MySQL执行异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        } catch (...) {
            conn.invalidate();
            LOGGER_ERROR("执行数据库操作时发生未知异常");
            throw;
        }
    }
    
    // 使用连接执行事务
    template<typename Func>
    auto execute_transaction(Func&& func) -> decltype(func(std::declval<MYSQL*>()))
    {
        auto conn = fetch();
        if (conn.broken()) {
            throw std::runtime_error("无法获取有效的数据库连接进行事务操作");
        }
        
        try {
            if (!conn.start_transaction()) {
                throw std::runtime_error("无法开始事务");
            }
            
            // 执行事务操作
            auto result = func(conn.get_mysql());
            
            if (!conn.commit()) {
                conn.rollback();
                throw std::runtime_error("提交事务失败");
            }
            conn.update_active_time();
            release(std::move(conn));
            return result;
        } catch (const std::exception& e) {
            LOGGER_ERROR(std::format("MySQL事务异常: {}", e.what()));
            conn.rollback();
            release(std::move(conn));
            throw;
        } catch (...) {
            LOGGER_ERROR("执行事务操作时发生未知异常，执行回滚");
            conn.rollback();
            release(std::move(conn));
            throw;
        }
    }

private:
    // 移动操作辅助函数
    void _move(ConnectionPool&& that) noexcept;
    
    // 从连接池获取连接的内部实现
    Connection _fetch(std::unique_lock<std::mutex>& lock);
    Connection _fetch();
    
    // 等待可用连接
    void _wait_for_connection(std::unique_lock<std::mutex>& lock);
    
    // 检查连接是否需要重连
    bool _need_reconnect(const Connection& connection) const;
    
    // 监控线程函数
    void _monitor_thread_func();
    
    // 连接选项
    ConnectionOptions _connection_opts;
    
    // 连接池选项
    ConnectionPoolOptions _pool_opts;
    
    // 连接池
    std::deque<Connection> _pool;
    
    // 已使用的连接数
    std::size_t _used_connections = 0;
    
    // 互斥锁
    mutable std::mutex _mutex;
    
    // 条件变量，用于等待可用连接
    std::condition_variable _cv;

    // 是否已关闭
    std::atomic<bool> _is_shutdown{false};
    
    // 监控线程
    std::thread _monitor_thread;
};

// 安全连接类，自动管理连接的获取和释放
class SafeConnection {
public:
    explicit SafeConnection(ConnectionPool& pool)
        : _pool(&pool), _connection(pool.fetch()) {}
    
    // 禁止拷贝
    SafeConnection(const SafeConnection&) = delete;
    SafeConnection& operator=(const SafeConnection&) = delete;
    
    // 禁止移动
    SafeConnection(SafeConnection&&) = delete;
    SafeConnection& operator=(SafeConnection&&) = delete;
    
    // 析构函数，自动释放连接
    ~SafeConnection() {
        if (_pool) {
            _pool->release(std::move(_connection));
        }
    }
    
    // 获取连接
    Connection& connection() {
        return _connection;
    }
    
    // 获取MYSQL指针
    MYSQL* mysql() {
        return _connection.get_mysql();
    }
    
 private:
    ConnectionPool* _pool;
    Connection _connection;
};

// 使用共享指针的连接守卫类
class GuardedConnection {
public:
    explicit GuardedConnection(const std::shared_ptr<ConnectionPool>& pool)
        : _pool(pool), _connection(_pool->fetch()) {}
    
    // 禁止拷贝
    GuardedConnection(const GuardedConnection&) = delete;
    GuardedConnection& operator=(const GuardedConnection&) = delete;
    
    // 允许移动
    GuardedConnection(GuardedConnection&&) = default;
    GuardedConnection& operator=(GuardedConnection&&) = default;
    
    // 析构函数，自动释放连接
    ~GuardedConnection() {
        // 如果GuardedConnection已被移动，_pool将为nullptr
        if (_pool) {
            _pool->release(std::move(_connection));
        }
    }
    
    // 获取连接
    Connection& connection() {
        return _connection;
    }
    
    // 获取MYSQL指针
    MYSQL* mysql() {
        return _connection.get_mysql();
    }
    
private:
    std::shared_ptr<ConnectionPool> _pool;
    Connection _connection;
};

// 共享指针类型别名
using ConnectionPoolSharedPtr = std::shared_ptr<ConnectionPool>;
using ConnectionPoolPtr = std::unique_ptr<ConnectionPool>;
using ConnectionPtr = std::unique_ptr<Connection>;
using GuardedConnectionSharedPtr = std::shared_ptr<GuardedConnection>;

class ConnectionPoolManager {
    SINGLETON_CREATE_H(ConnectionPoolManager)
public:
    ConnectionPoolSharedPtr get_pool(ConnectionOptions conn_opts,
        ConnectionPoolOptions pool_opts = ConnectionPoolOptions{}) const
    {
        // 生成唯一键
        std::string key = _generate_key(conn_opts, pool_opts);

        std::lock_guard<std::mutex> lock(_mutex);

        // 查找或创建连接池
        auto it = _pools.find(key);
        if (it != _pools.end()) {
            return it->second;
        }

        // 创建新连接池
        auto pool = std::make_shared<ConnectionPool>(std::move(conn_opts), std::move(pool_opts));
        _pools.emplace(key, pool);

        return pool;
    }

    void shutdown_all() {
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto& pair : _pools) {
            try {
                pair.second->shutdown();
                LOGGER_INFO(std::format("关闭连接池: {}", pair.first));
            }
            catch (const std::exception& e) {
                LOGGER_ERROR(std::format("关闭连接池失败: {}", e.what()));
            }
        }
        _pools.clear();
    }
private:
    ConnectionPoolManager() = default;
    ~ConnectionPoolManager() = default;

    std::string _generate_key(const ConnectionOptions& conn_opts,
        const ConnectionPoolOptions& pool_opts) const {
        return std::format("{}:{}:{}",
            conn_opts.host,
            conn_opts.port,
            conn_opts.database
        );
    }
    mutable std::mutex _mutex;
    mutable std::unordered_map<std::string, ConnectionPoolSharedPtr> _pools;
};
} // namespace universal::mysql

#endif // !UNIVERSAL_MYSQL_CONNECTION_POOL_H_