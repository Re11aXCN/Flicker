/*************************************************************************************
 *
 * @ Filename     : MySQLConnection.h
 * @ Description : MySQL连接封装类，使用纯C API
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
#ifndef MYSQL_CONNECTION_H_
#define MYSQL_CONNECTION_H_

#include <string>
#include <chrono>
#include <mutex>
#include <mysql.h>

class MySQLConnection {
public:
    explicit MySQLConnection(MYSQL* mysql);
    ~MySQLConnection();

    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&) = delete;
    MySQLConnection(MySQLConnection&&) = delete;
    MySQLConnection& operator=(MySQLConnection&&) = delete;

    MYSQL* getMysql();
    void updateActiveTime();
    std::chrono::steady_clock::time_point getLastActiveTime() const;
    bool isExpired(std::chrono::milliseconds timeout) const;

    bool executeQuery(const std::string& query);
    bool startTransaction();
    bool commit();
    bool rollback();

    // 检查连接是否有效
    bool isValid();
private:
    MYSQL* _mysql;
    std::chrono::steady_clock::time_point _lastActiveTime;
    mutable std::mutex _mutex;
};

#endif // !MYSQL_CONNECTION_H_