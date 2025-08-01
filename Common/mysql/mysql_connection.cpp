#include "Common/mysql/mysql_connection.h"
                
#include "Common/logger/logger_defend.h"


MySQLConnection::MySQLConnection(MYSQL* mysql)
    : _mysql(mysql), _lastActiveTime(std::chrono::steady_clock::now()) {
}

MySQLConnection::~MySQLConnection() {
    try {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_mysql) {
            mysql_close(_mysql);
            _mysql = nullptr;
        }
    } catch (...) {
        // 析构函数中不应抛出异常，但应记录错误
        LOGGER_ERROR("MySQLConnection析构函数发生异常");
    }
}

MYSQL* MySQLConnection::getMysql() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _mysql;
}

void MySQLConnection::updateActiveTime() {
    std::lock_guard<std::mutex> lock(_mutex);
    _lastActiveTime = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point MySQLConnection::getLastActiveTime() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _lastActiveTime;
}

bool MySQLConnection::isExpired(std::chrono::milliseconds timeout) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastActiveTime) > timeout;
}

bool MySQLConnection::executeQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_mysql) {
        LOGGER_ERROR("MySQL连接无效");
        return false;
    }
    /*
    * SUCCESS 0
    * CR_COMMANDS_OUT_OF_SYNC
    * CRLOGGER_GONE_ERROR
    * CRLOGGER_LOST
    * CR_UNKNOWN_ERROR
    */
    try {
        int state = mysql_query(_mysql, query.c_str());
        if (state != 0) {
            std::string error = mysql_error(_mysql);
            LOGGER_ERROR(std::format("执行查询失败: {}", error));
            return false;
        }
        
        MYSQL_RES* result = mysql_store_result(_mysql);
        if (result) {
            mysql_free_result(result);
        }
        
        // 更新最后活动时间
        _lastActiveTime = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("执行查询时发生异常: {}", e.what()));
        return false;
    } catch (...) {
        LOGGER_ERROR("执行查询时发生未知异常");
        return false;
    }
}

bool MySQLConnection::startTransaction() {
    // executeQuery已经包含了锁和异常处理
    return executeQuery("START TRANSACTION");
}

bool MySQLConnection::commit() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_mysql) {
        LOGGER_ERROR("MySQL连接无效");
        return false;
    }
    
    try {
        if (mysql_commit(_mysql) != 0) {
            std::string error = mysql_error(_mysql);
            LOGGER_ERROR(std::format("提交事务失败: {}", error));
            return false;
        }
        
        // 更新最后活动时间
        _lastActiveTime = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("提交事务时发生异常: {}", e.what()));
        return false;
    } catch (...) {
        LOGGER_ERROR("提交事务时发生未知异常");
        return false;
    }
}

bool MySQLConnection::rollback() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_mysql) {
        LOGGER_ERROR("MySQL连接无效");
        return false;
    }
    
    try {
        if (mysql_rollback(_mysql) != 0) {
            std::string error = mysql_error(_mysql);
            LOGGER_ERROR(std::format("回滚事务失败: {}", error));
            return false;
        }
        
        // 更新最后活动时间
        _lastActiveTime = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception& e) {
        LOGGER_ERROR(std::format("回滚事务时发生异常: {}", e.what()));
        return false;
    } catch (...) {
        LOGGER_ERROR("回滚事务时发生未知异常");
        return false;
    }
}

bool MySQLConnection::isValid() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_mysql) {
        return false;
    }
    
    try {
        // 执行简单查询检查连接是否有效
        int state = mysql_query(_mysql, "SELECT 1");
        if (state != 0) {
            return false;
        }
        
        MYSQL_RES* result = mysql_store_result(_mysql);
        if (result) {
            mysql_free_result(result);
        }
        
        // 更新最后活动时间
        _lastActiveTime = std::chrono::steady_clock::now();
        return true;
    } catch (...) {
        // 任何异常都表示连接无效
        return false;
    }
}