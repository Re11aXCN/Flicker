#include "FKMySQLConnectionPool.h"
#include <sstream>
#include "Source/FKConfigManager.h"

SINGLETON_CREATE_CPP(FKMySQLConnectionPool)

FKMySQLConnectionPool::FKMySQLConnectionPool() {
	_initialize(FKConfigManager::getInstance()->getMySQLConnectionString());
	std::println("FKMySQLConnectionPool 已创建");
}

FKMySQLConnectionPool::~FKMySQLConnectionPool() {
	shutdown();
	std::println("FKMySQLConnectionPool 已销毁");
}

void FKMySQLConnectionPool::_initialize(const FKMySQLConfig& config) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsInitialized) {
        std::println("MySQL连接池已初始化，请先关闭再重新初始化");
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
        _pIsShutdown = false;
        
        // 启动监控线程
        _pMonitorThread = std::thread(&FKMySQLConnectionPool::_monitorThreadFunc, this);
        
        std::println("MySQL连接池初始化成功，连接数: {}", _pConnectionPool.size());
    } catch (const std::exception& e) {
        std::println("MySQL连接池初始化失败: {}", e.what());
        shutdown();
        throw;
    }
}

void FKMySQLConnectionPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(_pMutex);
        
        if (_pIsShutdown) {
            return;
        }
        
        _pIsShutdown = true;
        _pIsInitialized = false;
    }
    
    // 通知监控线程退出
    _pCondVar.notify_all();
    
    // 等待监控线程结束
    if (_pMonitorThread.joinable()) {
        _pMonitorThread.join();
    }
    
    // 清空连接池
    std::lock_guard<std::mutex> lock(_pMutex);
    while (!_pConnectionPool.empty()) {
        _pConnectionPool.pop();
    }
    
    std::println("MySQL连接池已关闭");
}

std::shared_ptr<FKMySQLConnection> FKMySQLConnectionPool::_createConnection() {
    try {
        // 创建MySQL会话
        mysqlx::SessionSettings settings(_pConfig.Host, _pConfig.Port, _pConfig.Username, _pConfig.Password);
        settings.set(mysqlx::SessionOption::DB, _pConfig.Schema);
        settings.set(mysqlx::SessionOption::CONNECT_TIMEOUT, static_cast<unsigned int>(_pConfig.ConnectionTimeout.count()));
        
        // 创建会话
        auto session = std::make_unique<mysqlx::Session>(settings);
        
        // 测试连接
        session->sql("SELECT 1").execute();
        
        // 创建连接包装对象
        return std::make_shared<FKMySQLConnection>(std::move(session));
    } catch (const std::exception& e) {
        std::println("创建MySQL连接失败: {}", e.what());
        throw;
    }
}

std::shared_ptr<FKMySQLConnection> FKMySQLConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (!_pIsInitialized || _pIsShutdown) {
        throw std::runtime_error("MySQL连接池未初始化或已关闭");
    }
    
    // 检查当前时间与上次操作时间的差值
    auto now = std::chrono::steady_clock::now();
    if (!_pConnectionPool.empty()) {
        auto conn = _pConnectionPool.front();
        auto lastActiveTime = conn->getLastActiveTime();
        auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - lastActiveTime);
        
        // 如果时间差小于5秒，直接返回该连接
        if (timeDiff.count() < 5) {
            _pConnectionPool.pop();
            return conn;
        }
    }
    
    if (_pConnectionPool.empty()) {
        // 连接池为空，创建新连接
        std::println("MySQL连接池已用尽，创建新连接");
        return _createConnection();
    }
    
    // 从池中获取连接
    auto conn = _pConnectionPool.front();
    _pConnectionPool.pop();
    
    try {
        // 检查连接是否有效
        conn->getSession()->sql("SELECT 1").execute();
        return conn;
    } catch (const std::exception& e) {
        std::println("连接已失效，创建新连接: {}", e.what());
        return _createConnection();
    }
}

void FKMySQLConnectionPool::releaseConnection(std::shared_ptr<FKMySQLConnection> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsShutdown) {
        // 连接池已关闭，不再接受连接
        return;
    }
    
    // 更新最后活动时间
    conn->updateActiveTime();
    
    // 将连接放回池中
    _pConnectionPool.push(std::move(conn));
}

void FKMySQLConnectionPool::_monitorThreadFunc() {
    std::println("MySQL连接池监控线程已启动");
    
    while (!_pIsShutdown) {
        // 等待监控间隔时间或者收到关闭通知
        {
            std::unique_lock<std::mutex> lock(_pMutex);
            if (_pCondVar.wait_for(lock, _pConfig.MonitorInterval, [this] { return _pIsShutdown.load(); })) {
                // 收到关闭通知
                break;
            }
            
            if (_pConnectionPool.empty()) {
                continue;
            }
            
            // 检查并关闭超时连接
            size_t initialSize = _pConnectionPool.size();
            std::queue<std::shared_ptr<FKMySQLConnection>> tempQueue;
            
            // 遍历连接池中的所有连接
            while (!_pConnectionPool.empty()) {
                auto conn = _pConnectionPool.front();
                _pConnectionPool.pop();
                
                // 检查连接是否超时
                if (conn->isExpired(_pConfig.IdleTimeout)) {
                    // 连接已超时，不放回池中
                    std::println("关闭超时MySQL连接");
                } else {
                    // 连接未超时，放回临时队列
                    tempQueue.push(std::move(conn));
                }
            }
            
            // 将临时队列中的连接放回连接池
            _pConnectionPool = std::move(tempQueue);
            
            if (initialSize != _pConnectionPool.size()) {
                std::println("MySQL连接池监控: 关闭了 {} 个超时连接，当前连接数: {}", 
                    initialSize - _pConnectionPool.size(), _pConnectionPool.size());
            }
        }
    }
    
    std::println("MySQL连接池监控线程已退出");
}

std::string FKMySQLConnectionPool::getStatus() const {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    std::ostringstream oss;
    oss << "MySQL连接池状态:\n";
    oss << "================================\n";
    oss << "初始化: " << (_pIsInitialized ? "是" : "否") << "\n";
    oss << "已关闭: " << (_pIsShutdown ? "是" : "否") << "\n";
    oss << "主机: " << _pConfig.Host << "\n";
    oss << "端口: " << _pConfig.Port << "\n";
    oss << "数据库: " << _pConfig.Schema << "\n";
    oss << "连接池大小: " << _pConfig.PoolSize << "\n";
    oss << "可用连接数: " << _pConnectionPool.size() << "\n";
    oss << "连接超时: " << _pConfig.ConnectionTimeout.count() << "秒\n";
    oss << "空闲超时: " << _pConfig.IdleTimeout.count() << "秒\n";
    oss << "监控间隔: " << _pConfig.MonitorInterval.count() << "秒\n";
    oss << "================================\n";
    
    return oss.str();
}