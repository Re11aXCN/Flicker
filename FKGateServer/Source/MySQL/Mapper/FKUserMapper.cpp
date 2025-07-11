#include "FKUserMapper.h"
#include <print>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <regex>
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

// 创建用户表（如果不存在）
bool FKUserMapper::createTableIfNotExists() {
    return _pConnectionPool->executeWithConnection([this](MYSQL* mysql) {
        try {
            // 创建用户表SQL
            std::string createTableSQL = R"(
                CREATE TABLE IF NOT EXISTS )"
                + _tableName + R"( (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    uuid VARCHAR(36) NOT NULL UNIQUE,
                    username VARCHAR(50) NOT NULL UNIQUE,
                    email VARCHAR(100) NOT NULL UNIQUE,
                    password VARCHAR(255) NOT NULL, 
                    salt VARCHAR(29) NOT NULL,      
                    create_time TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3),
                    update_time TIMESTAMP(3) NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP(3),
                    INDEX idx_email (email),
                    INDEX idx_username (username)
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
            )";

            // 执行SQL
            if (mysql_query(mysql, createTableSQL.c_str())) {
                std::println("创建用户表失败: {}", mysql_error(mysql));
                return false;
            }
            
            std::println("用户表创建成功或已存在");
            return true;
        } catch (const std::exception& e) {
            std::println("创建用户表失败: {}", e.what());
            return false;
        }
    });
}

// 插入用户
bool FKUserMapper::insertUser(FKUserEntity& user) {
    return _pConnectionPool->executeWithConnection([&user, this](MYSQL* mysql) {
        try {
            // 构建插入SQL语句
            std::stringstream ss;
            ss << "INSERT INTO " << _tableName << " (uuid, username, email, password, salt) VALUES "
               << "('" << user.getUuid() << "', '"
               << mysql_real_escape_string(mysql, nullptr, user.getUsername().c_str(), user.getUsername().length()) << "', '"
               << mysql_real_escape_string(mysql, nullptr, user.getEmail().c_str(), user.getEmail().length()) << "', '"
               << mysql_real_escape_string(mysql, nullptr, user.getPassword().c_str(), user.getPassword().length()) << "', '"
               << mysql_real_escape_string(mysql, nullptr, user.getSalt().c_str(), user.getSalt().length()) << "')"; 

            // 执行SQL
            if (mysql_query(mysql, ss.str().c_str())) {
                std::println("插入用户失败: {}", mysql_error(mysql));
                return false;
            }

            // 获取自增ID
            user.setId(static_cast<int>(mysql_insert_id(mysql)));

            std::println("用户插入成功，ID: {}", user.getId());
            return true;
        } catch (const std::exception& e) {
            std::println("插入用户失败: {}", e.what());
            return false;
        }
    });
}

// 通用查询方法，根据条件查询单个用户
std::optional<FKUserEntity> FKUserMapper::_findUserByCondition(const std::string& whereClause, 
                                                             const std::string& paramValue) {
    return _pConnectionPool->executeWithConnection([this, whereClause, paramValue](MYSQL* mysql) {
        try {
            // 构建查询SQL
            std::stringstream ss;
            ss << "SELECT * FROM " << _tableName << " WHERE " << whereClause;
            
            // 执行查询
            if (mysql_query(mysql, ss.str().c_str())) {
                std::println("查询用户失败: {}", mysql_error(mysql));
                return std::optional<FKUserEntity>();
            }
            
            // 获取结果集
            MYSQL_RES* result = mysql_store_result(mysql);
            if (!result) {
                std::println("获取结果集失败: {}", mysql_error(mysql));
                return std::optional<FKUserEntity>();
            }
            
            // 获取行数据
            MYSQL_ROW row = mysql_fetch_row(result);
            if (!row) {
                mysql_free_result(result);
                return std::optional<FKUserEntity>();
            }
            
            // 构建用户实体
            FKUserEntity user = _buildUserFromRow(result, row);
            mysql_free_result(result);
            return std::optional<FKUserEntity>(user);
        } catch (const std::exception& e) {
            std::println("查询用户失败 ({}): {}", whereClause, e.what());
            return std::optional<FKUserEntity>();
        }
    });
}

// 根据ID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserById(int id) {
    std::string whereClause = "id = " + std::to_string(id);
    auto result = _findUserByCondition(whereClause, std::to_string(id));
    if (!result) {
        std::println("根据ID查询用户失败: {}", id);
    }
    return result;
}

// 根据UUID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUuid(const std::string& uuid) {
    std::string whereClause = "uuid = '" + uuid + "'";
    auto result = _findUserByCondition(whereClause, uuid);
    if (!result) {
        std::println("根据UUID查询用户失败: {}", uuid);
    }
    return result;
}

// 根据用户名查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUsername(const std::string& username) {
    std::string whereClause = "username = '" + username + "'";
    auto result = _findUserByCondition(whereClause, username);
    if (!result) {
        std::println("根据用户名查询用户失败: {}", username);
    }
    return result;
}

// 根据邮箱查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByEmail(const std::string& email) {
    std::string whereClause = "email = '" + email + "'";
    auto result = _findUserByCondition(whereClause, email);
    if (!result) {
        std::println("根据邮箱查询用户失败: {}", email);
    }
    return result;
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
               << "password = '" << mysql_real_escape_string(mysql, nullptr, user.getPassword().c_str(), user.getPassword().length()) << "', "
               << "salt = '" << mysql_real_escape_string(mysql, nullptr, user.getSalt().c_str(), user.getSalt().length()) << "' "
               << "WHERE id = " << user.getId();

            // 执行SQL
            if (mysql_query(mysql, ss.str().c_str())) {
                std::println("更新用户失败: {}", mysql_error(mysql));
                return false;
            }

            // 获取影响的行数
            my_ulonglong affectedRows = mysql_affected_rows(mysql);
            std::println("用户更新成功，影响行数: {}", affectedRows);
            return affectedRows > 0;
        } catch (const std::exception& e) {
            std::println("更新用户失败: {}", e.what());
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
                std::println("删除用户失败: {}", mysql_error(mysql));
                return false;
            }

            // 获取影响的行数
            my_ulonglong affectedRows = mysql_affected_rows(mysql);
            std::println("用户删除成功，影响行数: {}", affectedRows);
            return affectedRows > 0;
        } catch (const std::exception& e) {
            std::println("删除用户失败: {}", e.what());
            return false;
        }
    });
}

// 查询所有用户
std::vector<FKUserEntity> FKUserMapper::findAllUsers() {
    return _pConnectionPool->executeWithConnection([this](MYSQL* mysql) {
        std::vector<FKUserEntity> users;

        try {
            // 构建查询SQL
            std::string sql = "SELECT * FROM " + _tableName;

            // 执行查询
            if (mysql_query(mysql, sql.c_str())) {
                std::println("查询所有用户失败: {}", mysql_error(mysql));
                return users;
            }

            // 获取结果集
            MYSQL_RES* result = mysql_store_result(mysql);
            if (!result) {
                std::println("获取结果集失败: {}", mysql_error(mysql));
                return users;
            }

            // 获取字段信息并创建映射
            std::unordered_map<std::string, int> columnMap;
            MYSQL_FIELD* fields = mysql_fetch_fields(result);
            unsigned int numFields = mysql_num_fields(result);
            
            for (unsigned int i = 0; i < numFields; i++) {
                columnMap[fields[i].name] = i;
            }

            // 处理结果
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                // 从行数据构建用户实体
                int id = row[columnMap["id"]] ? std::stoi(row[columnMap["id"]]) : 0;
                std::string uuid = row[columnMap["uuid"]] ? row[columnMap["uuid"]] : "";
                std::string username = row[columnMap["username"]] ? row[columnMap["username"]] : "";
                std::string email = row[columnMap["email"]] ? row[columnMap["email"]] : "";
                std::string password = row[columnMap["password"]] ? row[columnMap["password"]] : "";
                std::string salt = row[columnMap["salt"]] ? row[columnMap["salt"]] : "";
                
                // 处理时间字段
                std::chrono::system_clock::time_point createTime = std::chrono::system_clock::now(); // 默认为当前时间
                std::optional<std::chrono::system_clock::time_point> updateTime = std::nullopt;
                
                // 解析create_time
                if (row[columnMap["create_time"]] && strlen(row[columnMap["create_time"]]) > 0) {
                    // MySQL TIMESTAMP格式为"YYYY-MM-DD HH:MM:SS.fff"
                    std::string createTimeStr = row[columnMap["create_time"]];
                    createTime = parseTimestamp(createTimeStr);
                }
                
                // 解析update_time
                if (row[columnMap["update_time"]] && strlen(row[columnMap["update_time"]]) > 0) {
                    std::string updateTimeStr = row[columnMap["update_time"]];
                    updateTime = parseTimestamp(updateTimeStr);
                }
                
                users.emplace_back(id, uuid, username, email, password, salt, createTime, updateTime);
            }

            // 确保释放结果集
            mysql_free_result(result);
            std::println("查询到 {} 个用户", users.size());
        } catch (const std::exception& e) {
            std::println("查询所有用户失败: {}", e.what());
        }

        return users;
    });
}

// 解析MySQL时间戳字符串为std::chrono::system_clock::time_point
std::chrono::system_clock::time_point FKUserMapper::parseTimestamp(const std::string& timestampStr) {
    // MySQL TIMESTAMP(3)格式为"YYYY-MM-DD HH:MM:SS.fff"
    std::tm tm = {};
    int milliseconds = 0;
    
    // 使用正则表达式解析时间戳，包括可能的毫秒部分
    std::regex timestamp_regex(R"(([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\.([0-9]{1,3}))?)");
    std::smatch matches;
    
    if (std::regex_match(timestampStr, matches, timestamp_regex)) {
        // 解析年月日时分秒
        tm.tm_year = std::stoi(matches[1].str()) - 1900; // 年份需要减去1900
        tm.tm_mon = std::stoi(matches[2].str()) - 1;     // 月份从0开始
        tm.tm_mday = std::stoi(matches[3].str());
        tm.tm_hour = std::stoi(matches[4].str());
        tm.tm_min = std::stoi(matches[5].str());
        tm.tm_sec = std::stoi(matches[6].str());
        
        // 解析毫秒部分（如果存在）
        if (matches.size() > 7 && matches[7].matched) {
            std::string msStr = matches[7].str();
            // 根据毫秒字符串的长度进行适当的转换
            if (msStr.length() == 1) {
                milliseconds = std::stoi(msStr) * 100;
            } else if (msStr.length() == 2) {
                milliseconds = std::stoi(msStr) * 10;
            } else {
                milliseconds = std::stoi(msStr);
            }
        }
    } else {
        // 如果解析失败，返回当前时间
        return std::chrono::system_clock::now();
    }
    
    // 转换为time_t
    std::time_t time = std::mktime(&tm);
    
    // 转换为time_point并添加毫秒
    auto timePoint = std::chrono::system_clock::from_time_t(time);
    timePoint += std::chrono::milliseconds(milliseconds);
    
    return timePoint;
}

// 从结果集构建用户实体 - 使用列名而不是索引，提高代码健壮性
FKUserEntity FKUserMapper::_buildUserFromRow(MYSQL_RES* result, MYSQL_ROW row) {
    // 创建列名到索引的映射
    std::unordered_map<std::string, int> columnMap;
    MYSQL_FIELD* fields;
    unsigned int numFields = mysql_num_fields(result);
    
    // 获取字段信息并创建映射
    fields = mysql_fetch_fields(result);
    for (unsigned int i = 0; i < numFields; i++) {
        columnMap[fields[i].name] = i;
        std::cout << "列名：" << fields[i].name << "数据： " << (row[i] ? row[i] : "NULL") << std::endl;
    }
    
    // 使用列名获取数据，避免依赖列的顺序
    int id = row[columnMap["id"]] ? std::stoi(row[columnMap["id"]]) : 0;
    std::string uuid = row[columnMap["uuid"]] ? row[columnMap["uuid"]] : "";
    std::string username = row[columnMap["username"]] ? row[columnMap["username"]] : "";
    std::string email = row[columnMap["email"]] ? row[columnMap["email"]] : "";
    std::string password = row[columnMap["password"]] ? row[columnMap["password"]] : "";
    std::string salt = row[columnMap["salt"]] ? row[columnMap["salt"]] : "";
    
    // 处理时间字段
    std::chrono::system_clock::time_point createTime = std::chrono::system_clock::now(); // 默认为当前时间
    std::optional<std::chrono::system_clock::time_point> updateTime = std::nullopt;
    
    // 解析create_time
    if (columnMap.find("create_time") != columnMap.end() && 
        row[columnMap["create_time"]] && 
        strlen(row[columnMap["create_time"]]) > 0) {
        std::string createTimeStr = row[columnMap["create_time"]];
        createTime = parseTimestamp(createTimeStr);
    }
    
    // 解析update_time
    if (columnMap.find("update_time") != columnMap.end() && 
        row[columnMap["update_time"]] && 
        strlen(row[columnMap["update_time"]]) > 0) {
        std::string updateTimeStr = row[columnMap["update_time"]];
        updateTime = parseTimestamp(updateTimeStr);
    }

    return FKUserEntity(id, uuid, username, email, password, salt, createTime, updateTime);
}