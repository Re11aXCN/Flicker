#include "FKMySQLConnectionPool.h"
#include <print>
#include <sstream>
#include "Source/FKConfigManager.h"

SINGLETON_CREATE_CPP(FKMySQLConnectionPool)

FKMySQLConnectionPool::FKMySQLConnectionPool() {
    std::println("MySQL连接池已创建");
    
    try {
        // 从配置管理器获取MySQL配置
        auto& config = FKConfigManager::getInstance()->getMySQLConnectionString();
        _initialize(config);
    } catch (const std::exception& e) {
        std::println("初始化MySQL连接池失败: {}", e.what());
    }
}

FKMySQLConnectionPool::~FKMySQLConnectionPool() {
    shutdown();
    std::println("MySQL连接池已销毁");
}

void FKMySQLConnectionPool::_initialize(const FKMySQLConfig& config) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsInitialized) {
        std::println("MySQL连接池已经初始化");
        return;
    }
    
    _pConfig = config;
    
    // 初始化MySQL库
    if (mysql_library_init(0, nullptr, nullptr)) {
        std::println("MySQL库初始化失败");
        return;
    }
    
    // 创建初始连接
    for (size_t i = 0; i < _pConfig.PoolSize; ++i) {
        try {
            auto conn = _createConnection();
            if (conn) {
                _pConnectionPool.push(std::move(conn));
            }
        } catch (const std::exception& e) {
            std::println("创建MySQL连接失败: {}", e.what());
        }
    }
    
    std::println("MySQL连接池初始化完成，创建了 {} 个连接", _pConnectionPool.size());
    
    // 标记为已初始化
    _pIsInitialized = true;
    _pIsShutdown = false;
    
    // 启动监控线程
    _pMonitorThread = std::thread(&FKMySQLConnectionPool::_monitorThreadFunc, this);
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
    
    // 关闭MySQL库
    mysql_library_end();
    
    std::println("MySQL连接池已关闭");
}

std::shared_ptr<FKMySQLConnection> FKMySQLConnectionPool::_createConnection() {
    try {
        // 初始化MySQL连接
        MYSQL* mysql = mysql_init(nullptr);
        if (!mysql) {
            throw std::runtime_error("MySQL初始化失败");
        }
        
        // 设置连接选项
        unsigned int timeout = static_cast<unsigned int>(_pConfig.ConnectionTimeout.count());
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        

        // MySQL 8.0.34+版本已弃用MYSQL_OPT_RECONNECT
        // 使用会话变量设置自动重连
        // 注意：连接后需要执行SET SESSION wait_timeout和SET SESSION interactive_timeout
        // 来延长连接超时时间，防止连接断开
		// 设置自动重连
		//char reconnect = 1;
		//mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

        // 设置字符集
        mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        
        // 建立连接
        if (!mysql_real_connect(
                mysql, 
                _pConfig.Host.c_str(),
                _pConfig.Username.c_str(),
                _pConfig.Password.c_str(),
                _pConfig.Schema.c_str(),
                _pConfig.Port,
                nullptr,  // unix_socket
                0  // client_flag
            )) {
            std::string error = mysql_error(mysql);
            mysql_close(mysql);
            throw std::runtime_error("MySQL连接失败: " + error);
        }
        
        // 设置会话变量来延长连接超时时间，替代已弃用的MYSQL_OPT_RECONNECT
        if (mysql_query(mysql, "SET SESSION wait_timeout=28800") != 0) {
            std::println("设置wait_timeout失败: {}", mysql_error(mysql));
        }
        
        if (mysql_query(mysql, "SET SESSION interactive_timeout=28800") != 0) {
            std::println("设置interactive_timeout失败: {}", mysql_error(mysql));
        }
        
        // 创建连接包装对象
        return std::make_shared<FKMySQLConnection>(mysql);
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
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastActiveTime);
        
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
        if (conn->isValid()) {
            return conn;
        } else {
            std::println("连接已失效，创建新连接");
            return _createConnection();
        }
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
                    // conn的析构函数会自动关闭连接
                } else {
                    // 检查连接是否有效
                    try {
                        if (conn->isValid()) {
                            // 连接有效且未超时，放回临时队列
                            tempQueue.push(std::move(conn));
                        } else {
                            std::println("关闭无效MySQL连接");
                            // conn的析构函数会自动关闭连接
                        }
                    } catch (const std::exception& e) {
                        std::println("检查连接有效性异常: {}", e.what());
                        // 连接异常，不放回池中
                    }
                }
            }
            
            // 将临时队列中的连接放回连接池
            _pConnectionPool = std::move(tempQueue);
            
            if (initialSize != _pConnectionPool.size()) {
                std::println("MySQL连接池监控: 关闭了 {} 个超时或无效连接，当前连接数: {}", 
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
    oss << "用户名: " << _pConfig.Username << "\n";
    oss << "数据库: " << _pConfig.Schema << "\n";
    oss << "连接池大小: " << _pConfig.PoolSize << "\n";
    oss << "可用连接数: " << _pConnectionPool.size() << "\n";
    oss << "连接超时: " << _pConfig.ConnectionTimeout.count() << "毫秒\n";
    oss << "空闲超时: " << _pConfig.IdleTimeout.count() << "毫秒\n";
    oss << "监控间隔: " << _pConfig.MonitorInterval.count() << "毫秒\n";
    oss << "================================\n";
    
    return oss.str();
}