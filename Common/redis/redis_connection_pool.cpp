﻿#include "Common/redis/redis_connection_pool.h"
#include <sstream>
#include "Common/config/config_manager.h"

SINGLETON_CREATE_SHARED_CPP(RedisConnectionPool)

RedisConnectionPool::RedisConnectionPool() {
    // 初始化连接池
    _initialize(ConfigManager::getInstance()->getRedisConfig());
}

RedisConnectionPool::~RedisConnectionPool() {
    shutdown();
}

void RedisConnectionPool::_initialize(const RedisConfig& config) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsInitialized) {
        LOGGER_WARN("Redis连接池已初始化，请先关闭再重新初始化");
        return;
    }
    
    try {
        _pConfig = config;
        
        // 预创建连接
        for (size_t i = 0; i < _pConfig.PoolSize; ++i) {
            auto conn = _createConnection();
            if (conn) {
                _pConnectionPool.push(std::move(conn));
            }
        }
        
        _pIsInitialized = true;
        LOGGER_INFO(std::format("Redis连接池初始化成功，连接数: {}", _pConnectionPool.size()));
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("Redis连接池初始化失败: {}", e.what()));
        shutdown();
        throw;
    }
}

void RedisConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    // 清空连接池
    while (!_pConnectionPool.empty()) {
        _pConnectionPool.pop();
    }
    
    _pIsInitialized = false;
    LOGGER_INFO("Redis连接池已关闭");
}

std::unique_ptr<sw::redis::Redis> RedisConnectionPool::_createConnection() {
    try {
        // 构建连接URI
        std::ostringstream uri;
        uri << "tcp://" << _pConfig.Host << ":" << _pConfig.Port;
        
        // 创建连接选项
        sw::redis::ConnectionOptions options;
        options.host = _pConfig.Host;
        options.port = _pConfig.Port;
        options.password = _pConfig.Password;
        options.db = _pConfig.DBIndex;
        options.connect_timeout = _pConfig.ConnectionTimeout;
        options.socket_timeout = _pConfig.SocketTimeout;
        
        // 创建连接池选项
        sw::redis::ConnectionPoolOptions pool_options;
        pool_options.size = 1; // 每个Redis对象只有一个连接
        
        // 创建Redis连接
        auto redis = std::make_unique<sw::redis::Redis>(options, pool_options);
        
        // 测试连接
        redis->ping();
        
        return redis;
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("创建Redis连接失败: {}", e.what()));
        throw;
    }
}

std::unique_ptr<sw::redis::Redis> RedisConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (!_pIsInitialized) {
        throw std::runtime_error("Redis连接池未初始化");
    }
    
    if (_pConnectionPool.empty()) {
        // 连接池为空，创建新连接
        LOGGER_WARN("Redis连接池已用尽，创建新连接");
        return _createConnection();
    }
    
    // 从池中获取连接
    auto conn = std::move(_pConnectionPool.front());
    _pConnectionPool.pop();
    
    return conn;
}

void RedisConnectionPool::releaseConnection(std::unique_ptr<sw::redis::Redis> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (!_pIsInitialized) {
        // 连接池已关闭，不再接受连接
        return;
    }
    
    // 将连接放回池中
    _pConnectionPool.push(std::move(conn));
}

std::string RedisConnectionPool::getStatus() const {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    std::ostringstream oss;
    oss << "Redis连接池状态:\n";
    oss << "================================\n";
    oss << "初始化: " << (_pIsInitialized ? "是" : "否") << "\n";
    oss << "主机: " << _pConfig.Host << "\n";
    oss << "端口: " << _pConfig.Port << "\n";
    oss << "数据库索引: " << _pConfig.DBIndex << "\n";
    oss << "连接池大小: " << _pConfig.PoolSize << "\n";
    oss << "可用连接数: " << _pConnectionPool.size() << "\n";
    oss << "================================\n";
    
    return oss.str();
}