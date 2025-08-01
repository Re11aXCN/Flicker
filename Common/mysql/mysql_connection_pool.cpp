#include "Common/mysql/mysql_connection_pool.h"

#include "Common/utils/utils.h"
#include "Common/config/config_manager.h"

SINGLETON_CREATE_CPP(MySQLConnectionPool)
static const std::string read_default_file = utils::concat(utils::get_env_a<"MYSQL_HOME">(), utils::local_separator(), "data", utils::local_separator(), "my.ini");
static const std::string read_default_group =  "mysqld";

MySQLConnectionPool::MySQLConnectionPool() {
    try {
        auto& config = ConfigManager::getInstance()->getMySQLConnectionString();
        _initialize(config);
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("初始化MySQL连接池发生异常: {}", e.what()));
    }
}

MySQLConnectionPool::~MySQLConnectionPool() {
    shutdown();
}

void MySQLConnectionPool::_initialize(const MySQLConfig& config) {
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsInitialized) {
        LOGGER_WARN("MySQL连接池已经初始化");
        return;
    }
    
    _pConfig = config;
    
    // 初始化MySQL库
    if (mysql_library_init(0, nullptr, nullptr)) {
        LOGGER_ERROR("MySQL库初始化失败");
        return;
    }
    
    for (size_t i = 0; i < _pConfig.PoolSize; ++i) {
        auto conn = _createConnection();
        if (conn) {
            _pConnectionPool.push(std::move(conn));
        }
    }
    
    LOGGER_INFO(std::format("MySQL连接池初始化完成，创建了 {} 个连接", _pConnectionPool.size()));
    
    // 标记为已初始化
    _pIsInitialized = true;
    _pIsShutdown = false;
    
    // 启动监控线程
    _pMonitorThread = std::thread(&MySQLConnectionPool::_monitorThreadFunc, this);
}

void MySQLConnectionPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(_pMutex);
        if (_pIsShutdown) {
            return;
        }
        _pIsShutdown = true;
        _pIsInitialized = false;
    }
    
    try {
        // 通知监控线程退出
        _pCondVar.notify_all();
        
        // 等待监控线程结束
        if (_pMonitorThread.joinable()) {
            _pMonitorThread.join();
        }
        
        // 清空连接池
        std::lock_guard<std::mutex> lock(_pMutex);
        while (!_pConnectionPool.empty()) {
            auto conn = _pConnectionPool.front();
            _pConnectionPool.pop();
            // conn的析构函数会自动关闭连接
        }
        
        // 关闭MySQL库
        mysql_library_end();
        
        LOGGER_INFO("MySQL连接池已关闭");
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("关闭MySQL连接池时发生异常: {}", e.what()));
    } catch (...) {
        LOGGER_ERROR("关闭MySQL连接池时发生未知异常");
    }
}

std::shared_ptr<MySQLConnection> MySQLConnectionPool::_createConnection() {
    MYSQL* mysql = nullptr;
    try {
        // mysql_init() 函数内部申请了一片内存，返回了首地址。
        mysql = mysql_init(mysql);
        if (!mysql) {
            throw std::runtime_error("MySQL初始化失败");
        }
        /*unsigned long max_allowed_packet = 512 * 1024 * 1024;
        unsigned long net_buffer_length =1024;
        uint32_t connect_timeout = 10;
        uint32_t read_timeout = 30;
        uint32_t write_timeout = 60;
        uint32_t retry_count = 1;
        uint32_t compress = 1;
        uint32_t zstd_level = 7;*/

        /*mysql_options(mysql, MYSQL_OPT_RETRY_COUNT, &retry_count);
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
        mysql_options(mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
        mysql_options(mysql, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
        mysql_options(mysql, MYSQL_OPT_MAX_ALLOWED_PACKET, &max_allowed_packet);
        mysql_options(mysql, MYSQL_OPT_NET_BUFFER_LENGTH, &net_buffer_length);
        mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        mysql_options(mysql, MYSQL_OPT_COMPRESS, &compress);
        mysql_options(mysql, MYSQL_OPT_COMPRESSION_ALGORITHMS, "zlib,zstd,uncompressed");
        mysql_options(mysql, MYSQL_OPT_ZSTD_COMPRESSION_LEVEL, &zstd_level);*/
        //// MySQL 8.0.34+版本已弃用MYSQL_OPT_RECONNECT
        //// 使用会话变量设置自动重连
        //// 注意：连接后需要执行SET SESSION wait_timeout和SET SESSION interactive_timeout
        //// 来延长连接超时时间，防止连接断开
        //// 设置自动重连
        ////char reconnect = 1;
        ////mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

        mysql_options(mysql, MYSQL_READ_DEFAULT_FILE, read_default_file.c_str());
        mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, read_default_group.c_str());

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
            mysql = nullptr;
            throw std::runtime_error("MySQL连接失败: " + error);
        }

        // 创建连接包装对象
        return std::make_shared<MySQLConnection>(mysql);
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("创建MySQL连接发生异常: {}", e.what()));
        if (mysql) {
            mysql_close(mysql);
            mysql = nullptr;
        }
        throw;
    } catch (...) {
        LOGGER_ERROR("创建MySQL连接时发生未知异常");
        if (mysql) {
            mysql_close(mysql);
            mysql = nullptr;
        }
        throw std::runtime_error("创建MySQL连接时发生未知异常");
    }
}

std::shared_ptr<MySQLConnection> MySQLConnectionPool::getConnection() {
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
        LOGGER_WARN("MySQL连接池已用尽，创建新连接");
        try {
            return _createConnection();
        } catch (const std::exception& e) {
            LOGGER_ERROR(std::format("创建MySQL连接发生异常: {}", e.what()));
            throw; // 重新抛出异常，让调用者处理
        }
    }
    
    // 从池中获取连接
    auto conn = _pConnectionPool.front();
    _pConnectionPool.pop();
    
    try {
        // 检查连接是否有效
        if (conn->isValid()) {
            return conn;
        } 

        LOGGER_WARN("连接已失效，创建新连接");
        return _createConnection();
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("检查连接有效性发生异常: {}", e.what()));
        try {
            return _createConnection();
        } catch (const std::exception& e2) {
            LOGGER_ERROR(std::format("创建MySQL连接发生异常: {}", e2.what()));
            throw std::runtime_error("无法获取有效的数据库连接");
        }
    }
}

void MySQLConnectionPool::releaseConnection(std::shared_ptr<MySQLConnection> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(_pMutex);
    
    if (_pIsShutdown) {
        // 连接池已关闭，不再接受连接
        return;
    }
    
    try {
        // 检查连接是否有效
        if (conn->isValid()) {
            // 更新最后活动时间
            conn->updateActiveTime();
            
            // 将连接放回池中
            _pConnectionPool.push(std::move(conn));
        } else {
            LOGGER_WARN("释放无效连接，不放回连接池");
            // 无效连接不放回池中，conn的析构函数会自动关闭连接
        }
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("释放连接时发生异常: {}", e.what()));
        // 发生异常的连接不放回池中
    }
}

void MySQLConnectionPool::_monitorThreadFunc() {
    LOGGER_INFO("MySQL连接池监控线程已启动");
    while (!_pIsShutdown) {
        try {
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
                std::queue<std::shared_ptr<MySQLConnection>> tempQueue;
                
                // 遍历连接池中的所有连接
                while (!_pConnectionPool.empty()) {
                    auto conn = _pConnectionPool.front();
                    _pConnectionPool.pop();
                    
                    if (!conn) {
                        // 跳过空连接
                        continue;
                    }
                    
                    try {
                        // 检查连接是否超时
                        if (conn->isExpired(_pConfig.IdleTimeout)) {
                            // 连接已超时，不放回池中
                            LOGGER_WARN("关闭超时MySQL连接");
                            // conn的析构函数会自动关闭连接
                        } else {
                            // 检查连接是否有效
                            if (conn->isValid()) {
                                // 连接有效且未超时，放回临时队列
                                tempQueue.push(std::move(conn));
                            } else {
                                LOGGER_WARN("关闭无效MySQL连接");
                                // conn的析构函数会自动关闭连接
                            }
                        }
                    } catch (const std::exception& e) {
                        LOGGER_ERROR(std::format("检查连接有效性异常: {}", e.what()));
                        // 连接异常，不放回池中
                    }
                }
                
                // 将临时队列中的连接放回连接池
                _pConnectionPool = std::move(tempQueue);
                
                if (initialSize != _pConnectionPool.size()) {
                    LOGGER_INFO(std::format("MySQL连接池监控: 关闭了 {} 个超时或无效连接，当前连接数: {}", 
                        initialSize - _pConnectionPool.size(), _pConnectionPool.size()));
                }
            }
        } catch (const std::exception& e) {
            LOGGER_ERROR(std::format("MySQL连接池监控线程异常: {}", e.what()));
            // 短暂休眠，避免因异常导致的CPU占用过高
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (...) {
            LOGGER_ERROR("MySQL连接池监控线程发生未知异常");
            // 短暂休眠，避免因异常导致的CPU占用过高
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    LOGGER_INFO("MySQL连接池监控线程已退出");
}

std::string MySQLConnectionPool::getStatus() const {
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