#include "FKUserMapper.h"

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <regex>

#include "FKLogger.h"
#include "FKUtils.h"
#include "MySQL/Entity/FKUserEntity.h"
#include "MySQL/FKMySQLConnectionPool.h"
#include "MySQL/FKMySQLConnection.h"

FKUserMapper::FKUserMapper(const std::string& tableName /*= "users"*/)
    : _tableName(tableName), _pConnectionPool(FKMySQLConnectionPool::getInstance())
{
}

FKUserMapper::~FKUserMapper()
{

}

std::optional<FKUserEntity> FKUserMapper::findUserById(int id) {
    std::string whereClause = "id = " + std::to_string(id);
    auto result = _findUserByCondition(whereClause, "*");
    return result;
}

std::optional<FKUserEntity> FKUserMapper::findUserByUuid(const std::string& uuid) {
    std::string whereClause = "uuid = '" + uuid + "'";
    auto result = _findUserByCondition(whereClause, "*");
    return result;
}

std::optional<FKUserEntity> FKUserMapper::findUserByUsername(const std::string& username) {
    std::string whereClause = "username = '" + username + "'";
    auto result = _findUserByCondition(whereClause, "*");
    return result;
}

std::optional<FKUserEntity> FKUserMapper::findUserByEmail(const std::string& email) {
    std::string whereClause = "email = '" + email + "'";
    auto result = _findUserByCondition(whereClause, "*");
    return result;
}

bool FKUserMapper::createTableIfNotExists() {
    return _pConnectionPool->executeWithConnection([this](MYSQL* mysql) {
        try {
            // 创建用户表SQL
            std::string createTableSQL = R"(
                CREATE TABLE IF NOT EXISTS )"
                + _tableName + R"( (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    uuid VARCHAR(36) NOT NULL UNIQUE DEFAULT (UUID()),
                    username VARCHAR(36) NOT NULL UNIQUE,
                    email VARCHAR(255) NOT NULL UNIQUE,
                    password VARCHAR(60) NOT NULL,
                    create_time TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
                    update_time TIMESTAMP(3) NULL DEFAULT NULL,
                    INDEX idx_email (email),
                    INDEX idx_username (username)
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
            )";

            if (mysql_query(mysql, createTableSQL.c_str())) {
                FK_SERVER_ERROR(std::format("创建用户表失败: {}", mysql_error(mysql)));
                return false;
            }

            FK_SERVER_INFO("用户表创建成功或已存在");
            return true;
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("创建用户表发生一个未知错误: {}", e.what()));
            return false;
        }
        });
}

bool FKUserMapper::insertUser(FKUserEntity& user) {
    return _pConnectionPool->executeWithConnection([&user, this](MYSQL* mysql) {
        try {
            // 构建插入SQL语句
            std::stringstream ss;
            ss << "INSERT INTO " << _tableName << " (uuid, username, email, password) VALUES "
                << "('" << user.getUuid() << "', '"
                << mysql_real_escape_string(mysql, nullptr, user.getUsername().c_str(), user.getUsername().length()) << "', '"
                << mysql_real_escape_string(mysql, nullptr, user.getEmail().c_str(), user.getEmail().length()) << "', '"
                << mysql_real_escape_string(mysql, nullptr, user.getPassword().c_str(), user.getPassword().length()) << "')";

            // 执行SQL
            if (mysql_query(mysql, ss.str().c_str())) {
                FK_SERVER_ERROR(std::format("插入用户失败: {}", mysql_error(mysql)));
                return false;
            }

            // 获取自增ID
            user.setId(static_cast<int>(mysql_insert_id(mysql)));

            FK_SERVER_INFO(std::format("用户插入成功，ID: {}", user.getId()));
            return true;
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("插入用户发生一个未知错误: {}", e.what()));
            return false;
        }
        });
}

// 更新用户信息
bool FKUserMapper::updateUser(const FKUserEntity& user) {
    return _pConnectionPool->executeWithConnection([&user, this](MYSQL* mysql) {
        try {
            // 构建更新SQL
            std::stringstream ss;
            ss << "UPDATE " << _tableName << " SET "
                << "username = '" << mysql_real_escape_string(mysql, nullptr, user.getUsername().c_str(), user.getUsername().length()) << "', "
                << "email = '" << mysql_real_escape_string(mysql, nullptr, user.getEmail().c_str(), user.getEmail().length()) << "', "
                << "password = '" << mysql_real_escape_string(mysql, nullptr, user.getPassword().c_str(), user.getPassword().length()) << "' "
                << "WHERE id = " << user.getId();

            // 执行SQL
            if (mysql_query(mysql, ss.str().c_str())) {
                FK_SERVER_ERROR(std::format("更新用户失败: {}", mysql_error(mysql)));
                return false;
            }

            // 获取影响的行数
            my_ulonglong affectedRows = mysql_affected_rows(mysql);
            FK_SERVER_INFO(std::format("用户更新成功，影响行数: {}", affectedRows));
            return affectedRows > 0;
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("更新用户发生一个未知错误: {}", e.what()));
            return false;
        }
        });
}

// 删除用户
bool FKUserMapper::deleteUser(int id) {
    return _pConnectionPool->executeWithConnection([id, this](MYSQL* mysql) {
        try {
            // 构建删除SQL
            std::stringstream ss;
            ss << "DELETE FROM " << _tableName << " WHERE id = " << id;

            // 执行SQL
            if (mysql_query(mysql, ss.str().c_str())) {
                FK_SERVER_ERROR(std::format("删除用户失败: {}", mysql_error(mysql)));
                return false;
            }

            // 获取影响的行数
            my_ulonglong affectedRows = mysql_affected_rows(mysql);
            FK_SERVER_INFO(std::format("用户删除成功，影响行数: {}", affectedRows));
            return affectedRows > 0;
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("删除用户发生一个未知错误: {}", e.what()));
            return false;
        }
        });
}

//std::vector<FKUserEntity> FKUserMapper::findAllUsers() {
//    return _pConnectionPool->executeWithConnection([this](MYSQL* mysql) {
//        std::vector<FKUserEntity> users;
//
//        try {
//            std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
//
//            // 构建查询SQL
//            std::string sql = "SELECT * FROM " + _tableName;
//
//            // 执行查询
//            if (mysql_query(mysql, sql.c_str())) {
//                std::println("查询所有用户失败: {}", mysql_error(mysql));
//                return users;
//            }
//
//            // 获取结果集
//            MYSQL_RES* result = mysql_store_result(mysql);
//            if (!result) {
//                std::println("获取结果集失败: {}", mysql_error(mysql));
//                return users;
//            }
//
//            // 获取字段信息并创建映射
//            std::unordered_map<std::string, int> columnMap;
//            MYSQL_FIELD* fields = mysql_fetch_fields(result);
//            unsigned int numFields = mysql_num_fields(result);
//            
//            for (unsigned int i = 0; i < numFields; i++) {
//                columnMap[fields[i].name] = i;
//            }
//
//            // 处理结果
//            MYSQL_ROW row;
//            while ((row = mysql_fetch_row(result))) {
//                // 从行数据构建用户实体
//                int id = row[columnMap["id"]] ? std::stoi(row[columnMap["id"]]) : 0;
//                std::string uuid = row[columnMap["uuid"]] ? row[columnMap["uuid"]] : "";
//                std::string username = row[columnMap["username"]] ? row[columnMap["username"]] : "";
//                std::string email = row[columnMap["email"]] ? row[columnMap["email"]] : "";
//                std::string password = row[columnMap["password"]] ? row[columnMap["password"]] : "";
//                std::string salt = row[columnMap["salt"]] ? row[columnMap["salt"]] : "";
//                
//                // 处理时间字段
//                std::chrono::system_clock::time_point createTime = std::chrono::system_clock::now(); // 默认为当前时间
//                std::optional<std::chrono::system_clock::time_point> updateTime = std::nullopt;
//                
//                // 解析create_time
//                if (row[columnMap["create_time"]] && strlen(row[columnMap["create_time"]]) > 0) {
//                    // MySQL TIMESTAMP格式为"YYYY-MM-DD HH:MM:SS.fff"
//                    std::string createTimeStr = row[columnMap["create_time"]];
//                    createTime = parseTimestamp(createTimeStr);
//                }
//                
//                // 解析update_time
//                if (row[columnMap["update_time"]] && strlen(row[columnMap["update_time"]]) > 0) {
//                    std::string updateTimeStr = row[columnMap["update_time"]];
//                    updateTime = parseTimestamp(updateTimeStr);
//                }
//                
//                users.emplace_back(id, uuid, username, email, password, salt, createTime, updateTime);
//            }
//            std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
//            std::chrono::duration<double> elapsedSeconds = endTime - startTime;
//            // 确保释放结果集
//            mysql_free_result(result);
//            std::println("查询所有用户耗时: {} s", elapsedSeconds.count());
//
//            std::println("查询到 {} 个用户", users.size());
//        } catch (const std::exception& e) {
//            std::println("查询所有用户失败: {}", e.what());
//        }
//
//        return users;
//    });
//}

// 比进行解析字符串转换获取时间的效率快了一倍，总体性能提升了25%左右
std::vector<FKUserEntity> FKUserMapper::findAllUsers() {
    return _pConnectionPool->executeWithConnection([this](MYSQL* mysql) {
        std::vector<FKUserEntity> users;
        try {
            std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
            // 修改查询SQL，直接获取毫秒时间戳
            std::string sql =
                "SELECT id, uuid, username, email, password, "
                "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
                "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
                "FROM " + _tableName;

            if (mysql_query(mysql, sql.c_str())) {
                FK_SERVER_ERROR(std::format("查询所有用户失败: {}", mysql_error(mysql)));
                return users;
            }

            MYSQL_RES* result = mysql_store_result(mysql);
            if (!result) {
                FK_SERVER_CRITICAL("结果集为空，仅`SELECT`、`SHOW`、`DESCRIBE`、`EXPLAIN`操作能够返回结果集");
                return users;
            }

            // 创建列名到索引的映射
            std::unordered_map<std::string, int> columnMap;
            MYSQL_FIELD* fields = mysql_fetch_fields(result);
            unsigned int numFields = mysql_num_fields(result);
            for (unsigned int i = 0; i < numFields; i++) {
                columnMap[fields[i].name] = i;
            }

            // 处理结果
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                int id = row[columnMap["id"]] ? std::stoi(row[columnMap["id"]]) : 0;
                std::string uuid = row[columnMap["uuid"]] ? row[columnMap["uuid"]] : "";
                std::string username = row[columnMap["username"]] ? row[columnMap["username"]] : "";
                std::string email = row[columnMap["email"]] ? row[columnMap["email"]] : "";
                std::string password = row[columnMap["password"]] ? row[columnMap["password"]] : "";
                std::string salt = row[columnMap["salt"]] ? row[columnMap["salt"]] : "";

                // 直接转换毫秒时间戳为 time_point
                auto parseTimestamp = [](const char* ms_str) -> std::chrono::system_clock::time_point {
                    if (!ms_str) return std::chrono::system_clock::now();
                    try {
                        uint64_t ms = std::stoull(ms_str);
                        return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
                    }
                    catch (...) {
                        return std::chrono::system_clock::now();
                    }
                    };

                std::chrono::system_clock::time_point createTime =
                    parseTimestamp(row[columnMap["create_time_ms"]]);

                std::optional<std::chrono::system_clock::time_point> updateTime =
                    row[columnMap["update_time_ms"]] ?
                    std::optional(parseTimestamp(row[columnMap["update_time_ms"]])) :
                    std::nullopt;

                users.emplace_back(id, uuid, username, email, password, salt, createTime, updateTime);
            }
            std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
            mysql_free_result(result);
            FK_SERVER_INFO(std::format("查询到 {} 个用户, 共耗时: {} s", users.size(), std::chrono::duration<double>{endTime - startTime}.count()));
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("查询所有用户失败: {}", e.what()));
        }
        return users;
        });
}

// 解析MySQL时间戳字符串为std::chrono::system_clock::time_point
std::chrono::system_clock::time_point FKUserMapper::_parseTimestamp(const std::string& timestampStr) {
    // 使用C++23的chrono库直接解析ISO 8601格式的时间戳
    // MySQL TIMESTAMP(3)格式为"YYYY-MM-DD HH:MM:SS.fff"
    try {
        // 检查时间戳字符串是否为空
        if (timestampStr.empty()) {
            return std::chrono::system_clock::now();
        }

        // 将MySQL时间戳格式转换为ISO 8601格式 (YYYY-MM-DDTHH:MM:SS.sssZ)
        // 替换空格为T，并添加Z表示UTC时区
        std::string isoTimestamp = timestampStr;
        size_t spacePos = isoTimestamp.find(' ');
        if (spacePos != std::string::npos) {
            isoTimestamp[spacePos] = 'T';
        }

        // 确保时间戳有毫秒部分，如果没有则添加
        if (isoTimestamp.find('.') == std::string::npos) {
            isoTimestamp += ".000";
        }

        // 添加Z表示UTC时区
        if (isoTimestamp.back() != 'Z') {
            isoTimestamp += "Z";
        }

        // 使用C++20的chrono::parse直接解析ISO 8601格式
        std::chrono::system_clock::time_point tp;
        std::istringstream ss(isoTimestamp);
        ss >> std::chrono::parse("%FT%T%Z", tp);

        // 如果解析失败，回退到手动解析
        if (ss.fail()) {
            return _fallbackParseTimestamp(timestampStr);
        }

        return tp;
    }
    catch (...) {
        // 如果发生任何异常，回退到手动解析方法
        return _fallbackParseTimestamp(timestampStr);
    }
}

// 回退方法：手动解析MySQL时间戳字符串
std::chrono::system_clock::time_point FKUserMapper::_fallbackParseTimestamp(const std::string& timestampStr) {
    // 使用更高效的字符串分割方法而不是正则表达式
    MYSQL_TIME mysqlTime = {};

    // 初始化MYSQL_TIME结构
    mysqlTime.time_type = MYSQL_TIMESTAMP_DATETIME;
    mysqlTime.neg = false;

    // 分割日期和时间部分
    size_t dateTimeSeparator = timestampStr.find(' ');
    if (dateTimeSeparator == std::string::npos) {
        return std::chrono::system_clock::now(); // 格式错误，返回当前时间
    }

    // 解析日期部分 (YYYY-MM-DD)
    std::string datePart = timestampStr.substr(0, dateTimeSeparator);
    size_t firstDash = datePart.find('-');
    size_t secondDash = datePart.find('-', firstDash + 1);

    if (firstDash == std::string::npos || secondDash == std::string::npos) {
        return std::chrono::system_clock::now(); // 格式错误，返回当前时间
    }

    mysqlTime.year = std::stoi(datePart.substr(0, firstDash));
    mysqlTime.month = std::stoi(datePart.substr(firstDash + 1, secondDash - firstDash - 1));
    mysqlTime.day = std::stoi(datePart.substr(secondDash + 1));

    // 解析时间部分 (HH:MM:SS[.fff])
    std::string timePart = timestampStr.substr(dateTimeSeparator + 1);
    size_t firstColon = timePart.find(':');
    size_t secondColon = timePart.find(':', firstColon + 1);
    size_t dot = timePart.find('.');

    if (firstColon == std::string::npos || secondColon == std::string::npos) {
        return std::chrono::system_clock::now(); // 格式错误，返回当前时间
    }

    mysqlTime.hour = std::stoi(timePart.substr(0, firstColon));
    mysqlTime.minute = std::stoi(timePart.substr(firstColon + 1, secondColon - firstColon - 1));

    // 解析秒和毫秒
    if (dot != std::string::npos && dot > secondColon) {
        mysqlTime.second = std::stoi(timePart.substr(secondColon + 1, dot - secondColon - 1));

        // 解析毫秒并转换为微秒 (MySQL使用微秒)
        std::string msStr = timePart.substr(dot + 1);
        unsigned long microseconds = 0;

        if (!msStr.empty()) {
            microseconds = std::stoul(msStr);
            // 根据毫秒字符串的长度进行适当的转换为微秒
            if (msStr.length() == 1) {
                microseconds *= 100000; // 1位数字，如.5 -> 500000微秒
            }
            else if (msStr.length() == 2) {
                microseconds *= 10000;  // 2位数字，如.50 -> 500000微秒
            }
            else if (msStr.length() == 3) {
                microseconds *= 1000;   // 3位数字，如.500 -> 500000微秒
            }
        }

        mysqlTime.second_part = microseconds;
    }
    else {
        mysqlTime.second = std::stoi(timePart.substr(secondColon + 1));
        mysqlTime.second_part = 0;
    }

    // 转换MYSQL_TIME到std::tm
    std::tm timeinfo = {};
    timeinfo.tm_year = mysqlTime.year - 1900; // 年份需要减去1900
    timeinfo.tm_mon = mysqlTime.month - 1;    // 月份从0开始
    timeinfo.tm_mday = mysqlTime.day;
    timeinfo.tm_hour = mysqlTime.hour;
    timeinfo.tm_min = mysqlTime.minute;
    timeinfo.tm_sec = mysqlTime.second;
    timeinfo.tm_isdst = -1; // 让系统自动判断夏令时

    // 转换为time_t
    std::time_t time = std::mktime(&timeinfo);

    // 转换为time_point并添加微秒
    auto timePoint = std::chrono::system_clock::from_time_t(time);
    timePoint += std::chrono::microseconds(mysqlTime.second_part);

    return timePoint;
}

std::optional<FKUserEntity> FKUserMapper::_findUserByCondition(const std::string& whereClause,
    const std::string& paramValue) {
    return _pConnectionPool->executeWithConnection([this, whereClause, paramValue](MYSQL* mysql) {
        try {
            std::string sql = FKUtils::concat("SELECT ", paramValue, " FROM ", _tableName, " WHERE ", whereClause);
            // 执行查询
            if (mysql_query(mysql, sql.c_str())) {
                FK_SERVER_ERROR(std::format("查询用户失败: {}", mysql_error(mysql)));
                return std::optional<FKUserEntity>();
            }

            // 获取结果集
            MYSQL_RES* result = mysql_store_result(mysql);
            if (!result) {
                FK_SERVER_CRITICAL("结果集为空，仅`SELECT`、`SHOW`、`DESCRIBE`、`EXPLAIN`操作能够返回结果集");
                return std::optional<FKUserEntity>();
            }

            // 获取行数据
            MYSQL_ROW row = mysql_fetch_row(result);
            if (!row) {
                FK_SERVER_INFO("查询结果为空");
                mysql_free_result(result);
                return std::optional<FKUserEntity>();
            }

            // 构建用户实体
            FKUserEntity user = _buildUserFromRow(result, row);
            mysql_free_result(result);
            return std::optional<FKUserEntity>(user);
        }
        catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("查询用户发生一个未知错误: {}", e.what()));
            return std::optional<FKUserEntity>();
        }
        });
}

FKUserEntity FKUserMapper::_buildUserFromRow(MYSQL_RES* result, MYSQL_ROW row) {

    int id = row[columnMap["id"]] ? std::stoi(row[columnMap["id"]]) : 0;
    std::string uuid = row[columnMap["uuid"]] ? row[columnMap["uuid"]] : "";
    std::string username = row[columnMap["username"]] ? row[columnMap["username"]] : "";
    std::string email = row[columnMap["email"]] ? row[columnMap["email"]] : "";
    std::string password = row[columnMap["password"]] ? row[columnMap["password"]] : "";
    std::string salt = row[columnMap["salt"]] ? row[columnMap["salt"]] : "";

    // 直接转换毫秒时间戳为 time_point
    auto parseTimestamp = [](const char* ms_str) -> std::chrono::system_clock::time_point {
        if (!ms_str) return std::chrono::system_clock::now();
        try {
            uint64_t ms = std::stoull(ms_str);
            return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }
        catch (...) {
            return std::chrono::system_clock::now();
        }
        };

    std::chrono::system_clock::time_point createTime =
        parseTimestamp(row[columnMap["create_time_ms"]]);

    std::optional<std::chrono::system_clock::time_point> updateTime =
        row[columnMap["update_time_ms"]] ?
        std::optional(parseTimestamp(row[columnMap["update_time_ms"]])) :
        std::nullopt;

    return FKUserEntity(id, uuid, username, email, password, salt, createTime, updateTime);
}
