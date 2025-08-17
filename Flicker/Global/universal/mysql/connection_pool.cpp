/*************************************************************************************
 *
 * @ Filename     : connection_pool.cpp
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
#include "connection_pool.h"
#include <format>

namespace universal::mysql {

// 构造函数
ConnectionPool::ConnectionPool(ConnectionOptions connection_opts,
    ConnectionPoolOptions pool_opts)
    : _connection_opts(std::move(connection_opts)), _pool_opts(std::move(pool_opts)) {
    // 初始化MySQL库
    if (mysql_library_init(0, nullptr, nullptr) != 0) {
        throw std::runtime_error("MySQL库初始化失败");
    }
    
    if (_pool_opts.size == 0) {
        throw std::runtime_error("无法创建空连接池");
    }

    for (size_t i = 0; i < _pool_opts.size; ++i) {
        _pool.emplace_back(_connection_opts);
    }

    // 启动监控线程
    _monitor_thread = std::thread(&ConnectionPool::_monitor_thread_func, this);

    LOGGER_INFO(std::format("MySQL连接池初始化成功，初始连接数: {}", _pool.size()));
}

// 移动构造函数
ConnectionPool::ConnectionPool(ConnectionPool&& that) noexcept {
    std::lock_guard<std::mutex> lock(that._mutex);
    _move(std::move(that));
}

// 移动赋值运算符
ConnectionPool& ConnectionPool::operator=(ConnectionPool&& that) noexcept {
    if (this != &that) {
        std::lock(_mutex, that._mutex);
        std::lock_guard<std::mutex> lock_this(_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock_that(that._mutex, std::adopt_lock);
        
        // 关闭当前连接池
        shutdown();
        
        // 移动资源
        _move(std::move(that));
    }
    return *this;
}

// 析构函数
ConnectionPool::~ConnectionPool() {
    shutdown();
    mysql_library_end();
}

// 移动操作辅助函数
void ConnectionPool::_move(ConnectionPool&& that) noexcept {
    _connection_opts = std::move(that._connection_opts);
    _pool_opts = std::move(that._pool_opts);
    _pool = std::move(that._pool);
    _used_connections = that._used_connections;
    _is_shutdown.store(that._is_shutdown.load());
    
    // 移动监控线程
    if (that._monitor_thread.joinable()) {
        _monitor_thread = std::move(that._monitor_thread);
    }
    
    // 重置that的状态
    that._used_connections = 0;
    that._is_shutdown.store(true);
}

// 关闭连接池
void ConnectionPool::shutdown() {
    if (_is_shutdown.load()) {
        return;
    }
    
    _is_shutdown.store(true);
    
    // 通知所有等待的线程
    _cv.notify_all();
    
    // 等待监控线程结束
    if (_monitor_thread.joinable()) {
        _monitor_thread.join();
    }
    
    // 清空连接池
    std::unique_lock<std::mutex> lock(_mutex);
    _pool.clear();
    _used_connections = 0;
    
    LOGGER_INFO("MySQL连接池已关闭");
}

// 从连接池获取一个连接
Connection ConnectionPool::fetch() {
    std::unique_lock<std::mutex> lock(_mutex);
    auto connection = _fetch(lock);
    
    lock.unlock();
    
    // 检查连接是否需要重连
    if (_need_reconnect(connection)) {
        if (bool isFailed = !connection.reconnect()) {
            // 重连失败，返回连接到池中，稍后重试
            release(std::move(connection));
            LOGGER_ERROR(std::format("重连MySQL连接失败"));
        }
    }
    
    return connection;
}

// 从连接池获取连接的内部实现
Connection ConnectionPool::_fetch(std::unique_lock<std::mutex>& lock) {
    if (_is_shutdown.load()) {
        throw std::runtime_error("连接池已关闭");
    }
    
    if (_pool.empty()) {
        if (_used_connections == _pool_opts.size) {
            // 等待可用连接
            _wait_for_connection(lock);
        } else {
            ++_used_connections;
            
            // 延迟创建一个新连接，避免在持有锁的情况下连接
            return Connection(_connection_opts);
        }
    }
    
    // 连接池不为空
    return _fetch();
}

// 从非空连接池获取连接
Connection ConnectionPool::_fetch() {
    assert(!_pool.empty());
    
    auto connection = std::move(_pool.front());
    _pool.pop_front();
    ++_used_connections;
    
    return connection;
}

// 等待可用连接
void ConnectionPool::_wait_for_connection(std::unique_lock<std::mutex>& lock) {
    auto wait_timeout = _pool_opts.wait_timeout;
    
    if (wait_timeout > std::chrono::milliseconds(0)) {
        // 有超时限制，等待指定时间
        if (!_cv.wait_for(lock, wait_timeout, [this] { 
                return !this->_pool.empty() || this->_is_shutdown.load(); 
            })) 
        {
            throw std::runtime_error("等待连接超时: " + 
                std::to_string(wait_timeout.count()) + " 毫秒");
        }
    } else {
        // 无超时限制，一直等待
        _cv.wait(lock, [this] { 
            return !this->_pool.empty() || this->_is_shutdown.load(); 
        });
    }
    
    if (_is_shutdown.load()) {
        throw std::runtime_error("连接池已关闭");
    }
}

// 检查连接是否需要重连
bool ConnectionPool::_need_reconnect(const Connection& connection) const {
    if (connection.broken()) {
        return true;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto connection_lifetime = _pool_opts.connection_lifetime;
    auto connection_idle_time = _pool_opts.connection_idle_time;
    
    // 检查连接生命周期
    if (connection_lifetime > std::chrono::milliseconds(0)) {
        if (now - connection.create_time() > connection_lifetime) {
            return true;
        }
    }
    
    // 检查连接空闲时间
    if (connection_idle_time > std::chrono::milliseconds(0)) {
        if (now - connection.last_active() > connection_idle_time) {
            return true;
        }
    }
    
    return false;
}

// 释放连接回连接池
void ConnectionPool::release(Connection connection) {
    if (_is_shutdown.load()) {
        // 连接池已关闭，不需要释放连接
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (connection.broken()) {
            // 连接已损坏，不放回连接池
            --_used_connections;
            return;
        }
        
        // 放回连接池
        _pool.push_back(std::move(connection));
        --_used_connections;
    }
    
    // 通知等待的线程
    _cv.notify_one();
}

// 获取连接选项
ConnectionOptions ConnectionPool::connection_options() const {
    std::unique_lock<std::mutex> lock(_mutex);
    return _connection_opts;
}

// 克隆连接池
std::shared_ptr<ConnectionPool> ConnectionPool::clone() const {
    std::unique_lock<std::mutex> lock(_mutex);
    return std::make_shared<ConnectionPool>(_connection_opts, _pool_opts);
}

// 监控线程函数
void ConnectionPool::_monitor_thread_func() {
    while (!_is_shutdown.load()) {
        // 休眠指定时间
        std::this_thread::sleep_for(_pool_opts.monitor_interval);
        
        if (_is_shutdown.load()) {
            break;
        }
        
        std::unique_lock<std::mutex> lock(_mutex);

        // 检查空闲连接
        auto it = _pool.begin();
        while (it != _pool.end()) {
            // 检查连接是否需要重连
            if (_need_reconnect(*it)) {
                // 移除过期或空闲超时的连接
                it = _pool.erase(it);
            }
            else {
                // 检查连接是否有效
                if (!it->is_valid()) {
                    // 尝试重连
                    if (bool isFailed = !it->reconnect()) {
                        // 重连失败，移除连接
                        it = _pool.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }

        // 确保连接池中有足够的连接
        while (_pool.size() + _used_connections < _pool_opts.size) {
            _pool.emplace_back(_connection_opts);

        }
    }
}

// 获取连接池状态信息
std::string ConnectionPool::get_status() const {
    std::unique_lock<std::mutex> lock(_mutex);
    
    return std::format(
        "连接池状态:\n"
        "  总连接数: {}\n"
        "  已使用连接: {}\n"
        "  空闲连接: {}\n"
        "  是否已关闭: {}",
        _pool_opts.size,
        _used_connections,
        _pool.size(),
        _is_shutdown.load() ? "是" : "否"
    );
}


SINGLETON_CREATE_CPP(ConnectionPoolManager)
} // namespace universal::mysql