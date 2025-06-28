#include "FKUserMapper.h"
#include <print>
#include <unordered_map>

// 创建用户表（如果不存在）
bool FKUserMapper::createTableIfNotExists() {
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
                create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                update_time TIMESTAMP NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP,
                INDEX idx_email (email),
                INDEX idx_username (username)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
        )";

        // 执行SQL
        _session->sql(createTableSQL).execute();
        std::println("用户表创建成功或已存在");
        return true;
    } catch (const std::exception& e) {
        std::println("创建用户表失败: {}", e.what());
        return false;
    }
}

// 插入用户
bool FKUserMapper::insertUser(FKUserEntity& user) {
    try {
        // 获取表对象
        auto schema = _session->getSchema(""); // 使用当前会话的schema
        auto table = schema.getTable(_tableName);

        // 插入用户数据
        auto result = table.insert("uuid", "username", "email", "password", "salt")
            .values(user.getUuid(), user.getUsername(), user.getEmail(), user.getPassword(), user.getSalt())
            .execute();

        // 获取自增ID
        user.setId(static_cast<int>(result.getAutoIncrementValue()));

        std::println("用户插入成功，ID: {}", user.getId());
        return true;
    } catch (const std::exception& e) {
        std::println("插入用户失败: {}", e.what());
        return false;
    }
}

// 通用查询方法，根据条件查询单个用户
std::optional<FKUserEntity> FKUserMapper::_findUserByCondition(const std::string& whereClause, 
                                                             const std::string& paramName, 
                                                             const auto& paramValue) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询用户 - 使用fetchOne而不是fetchAll，因为id、uuid、用户名和邮箱都是唯一的
        auto result = table.select("*")
            .where(whereClause)
            .bind(paramName, paramValue)
            .execute();

        // 处理结果 - 直接使用fetchOne获取单条记录
        auto row = result.fetchOne();
        if (!row) {
            return std::nullopt;
        }
        return _buildUserFromRow(row);
    } catch (const std::exception& e) {
        std::println("查询用户失败 ({}={}): {}", whereClause, paramValue, e.what());
        return std::nullopt;
    }
}

// 根据ID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserById(int id) {
    auto result = _findUserByCondition("id = :id", "id", id);
    if (!result) {
        std::println("根据ID查询用户失败: {}", id);
    }
    return result;
}

// 根据UUID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUuid(const std::string& uuid) {
    auto result = _findUserByCondition("uuid = :uuid", "uuid", uuid);
    if (!result) {
        std::println("根据UUID查询用户失败: {}", uuid);
    }
    return result;
}

// 根据用户名查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUsername(const std::string& username) {
    auto result = _findUserByCondition("username = :username", "username", username);
    if (!result) {
        std::println("根据用户名查询用户失败: {}", username);
    }
    return result;
}

// 根据邮箱查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByEmail(const std::string& email) {
    auto result = _findUserByCondition("email = :email", "email", email);
    if (!result) {
        std::println("根据邮箱查询用户失败: {}", email);
    }
    return result;
}

// 更新用户信息
bool FKUserMapper::updateUser(const FKUserEntity& user) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 更新用户
        auto result = table.update()
            .set("username", user.getUsername())
            .set("email", user.getEmail())
            .set("password", user.getPassword())
            .set("salt", user.getSalt())
            .where("id = :id")
            .bind("id", user.getId())
            .execute();

        std::println("用户更新成功，影响行数: {}", result.getAffectedItemsCount());
        return result.getAffectedItemsCount() > 0;
    } catch (const std::exception& e) {
        std::println("更新用户失败: {}", e.what());
        return false;
    }
}

// 删除用户
bool FKUserMapper::deleteUser(int id) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 删除用户
        auto result = table.remove()
            .where("id = :id")
            .bind("id", id)
            .execute();

        std::println("用户删除成功，影响行数: {}", result.getAffectedItemsCount());
        return result.getAffectedItemsCount() > 0;
    } catch (const std::exception& e) {
        std::println("删除用户失败: {}", e.what());
        return false;
    }
}

// 查询所有用户
std::vector<FKUserEntity> FKUserMapper::findAllUsers() {
    std::vector<FKUserEntity> users;

    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询所有用户
        auto result = table.select("*").execute();

        // 处理结果
        auto rows = result.fetchAll();
        for (const auto& row : rows) {
            users.push_back(_buildUserFromRow(row));
        }

        std::println("查询到 {} 个用户", users.size());
    } catch (const std::exception& e) {
        std::println("查询所有用户失败: {}", e.what());
    }

    return users;
}

// 从结果集构建用户实体 - 使用列名而不是索引，提高代码健壮性
FKUserEntity FKUserMapper::_buildUserFromRow(const mysqlx::Row& row) {
    // 获取列索引映射
    const auto& columns = row.getColumns();
    
    // 创建列名到索引的映射
    std::unordered_map<std::string, size_t> columnMap;
    for (size_t i = 0; i < columns.size(); ++i) {
        columnMap[columns[i].getColumnName()] = i;
    }
    
    // 使用列名获取数据，避免依赖列的顺序
    int id = row[columnMap["id"]].get<int>();
    std::string uuid = row[columnMap["uuid"]].get<std::string>();
    std::string username = row[columnMap["username"]].get<std::string>();
    std::string email = row[columnMap["email"]].get<std::string>();
    std::string password = row[columnMap["password"]].get<std::string>();
    std::string salt = row[columnMap["salt"]].get<std::string>();
    
    // 处理时间字段 - 使用线程安全的方式
    auto parseTimeString = [](const std::string& timeStr) -> std::chrono::system_clock::time_point {
        std::tm tm = {};
        std::istringstream ss(timeStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        // 检查解析是否成功
        if (ss.fail()) {
            std::println("时间解析失败: {}", timeStr);
            return std::chrono::system_clock::now(); // 返回当前时间作为默认值
        }
        
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    };
    
    auto createTimeStr = row[columnMap["create_time"]].get<std::string>();
    auto createTime = parseTimeString(createTimeStr);
    
    std::optional<std::chrono::system_clock::time_point> updateTime;
    if (!row[columnMap["update_time"]].isNull()) {
        auto updateTimeStr = row[columnMap["update_time"]].get<std::string>();
        updateTime = parseTimeString(updateTimeStr);
    }
    
    return FKUserEntity(id, uuid, username, email, password, salt, createTime, updateTime);

}