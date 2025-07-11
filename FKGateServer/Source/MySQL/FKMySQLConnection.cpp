#include "FKMySQLConnection.h"
#include <print>

// 构造函数，接收一个已初始化的MYSQL指针
FKMySQLConnection::FKMySQLConnection(MYSQL* mysql)
    : _mysql(mysql), _lastActiveTime(std::chrono::steady_clock::now()) {
}

// 析构函数，负责关闭连接
FKMySQLConnection::~FKMySQLConnection() {
    if (_mysql) {
        mysql_close(_mysql);
        _mysql = nullptr;
    }
}

// 获取原始MYSQL连接指针
MYSQL* FKMySQLConnection::getMysql() {
    return _mysql;
}

// 更新最后活动时间
void FKMySQLConnection::updateActiveTime() {
    std::lock_guard<std::mutex> lock(_mutex);
    _lastActiveTime = std::chrono::steady_clock::now();
}

// 获取最后活动时间
std::chrono::steady_clock::time_point FKMySQLConnection::getLastActiveTime() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _lastActiveTime;
}

// 检查连接是否已过期
bool FKMySQLConnection::isExpired(std::chrono::milliseconds timeout) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastActiveTime) > timeout;
}

// 执行简单查询
bool FKMySQLConnection::executeQuery(const std::string& query) {
    if (!_mysql) {
        std::println("MySQL连接无效");
        return false;
    }
    /*
	* SUCCESS 0
    * CR_COMMANDS_OUT_OF_SYNC
    * CR_SERVER_GONE_ERROR
    * CR_SERVER_LOST
    * CR_UNKNOWN_ERROR
    */
    int state = mysql_query(_mysql, query.c_str());
    if (state != 0) {
        std::println("执行查询失败: {}", mysql_error(_mysql));
        return false;
    }
    mysql_free_result(mysql_store_result(_mysql));

    return true;
}

// 开始事务
bool FKMySQLConnection::startTransaction() {
    return executeQuery("START TRANSACTION");
}

// 提交事务
bool FKMySQLConnection::commit() {
    if (!_mysql) {
        std::println("MySQL连接无效");
        return false;
    }
    
    if (mysql_commit(_mysql) != 0) {
        std::println("提交事务失败: {}", mysql_error(_mysql));
        return false;
    }
    
    return true;
}

// 回滚事务
bool FKMySQLConnection::rollback() {
    if (!_mysql) {
        std::println("MySQL连接无效");
        return false;
    }
    
    if (mysql_rollback(_mysql) != 0) {
        std::println("回滚事务失败: {}", mysql_error(_mysql));
        return false;
    }
    
    return true;
}

// 检查连接是否有效
bool FKMySQLConnection::isValid() {
    if (!_mysql) {
        return false;
    }
    
    // 执行简单查询检查连接是否有效
    return executeQuery("SELECT 1");
}