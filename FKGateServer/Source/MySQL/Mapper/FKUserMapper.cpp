#include "FKUserMapper.h"
#include <print>

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

// 根据ID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserById(int id) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询用户
        auto result = table.select("*")
            .where("id = :id")
            .bind("id", id)
            .execute();

        // 处理结果
        auto results = result.fetchAll();
        std::vector<mysqlx::Row> rows{ results.begin(), results.end() };
        if (rows.empty()) {
            return std::nullopt;
        }

        return _buildUserFromRow(rows[0]);
    } catch (const std::exception& e) {
        std::println("根据ID查询用户失败: {}", e.what());
        return std::nullopt;
    }
}

// 根据UUID查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUuid(const std::string& uuid) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询用户
        auto result = table.select("*")
            .where("uuid = :uuid")
            .bind("uuid", uuid)
            .execute();

        // 处理结果
		auto results = result.fetchAll();
		std::vector<mysqlx::Row> rows{ results.begin(), results.end() };
        if (rows.empty()) {
            return std::nullopt;
        }

        return _buildUserFromRow(rows[0]);
    } catch (const std::exception& e) {
        std::println("根据UUID查询用户失败: {}", e.what());
        return std::nullopt;
    }
}

// 根据用户名查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByUsername(const std::string& username) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询用户
        auto result = table.select("*")
            .where("username = :username")
            .bind("username", username)
            .execute();

        // 处理结果
		auto results = result.fetchAll();
		std::vector<mysqlx::Row> rows{ results.begin(), results.end() };
        if (rows.empty()) {
            return std::nullopt;
        }

        return _buildUserFromRow(rows[0]);
    } catch (const std::exception& e) {
        std::println("根据用户名查询用户失败: {}", e.what());
        return std::nullopt;
    }
}

// 根据邮箱查询用户
std::optional<FKUserEntity> FKUserMapper::findUserByEmail(const std::string& email) {
    try {
        // 获取表对象
        auto schema = _session->getSchema("");
        auto table = schema.getTable(_tableName);

        // 查询用户
        auto result = table.select("*")
            .where("email = :email")
            .bind("email", email)
            .execute();

        // 处理结果
		auto results = result.fetchAll();
		std::vector<mysqlx::Row> rows{ results.begin(), results.end() };
        if (rows.empty()) {
            return std::nullopt;
        }

        return _buildUserFromRow(rows[0]);
    } catch (const std::exception& e) {
        std::println("根据邮箱查询用户失败: {}", e.what());
        return std::nullopt;
    }
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

// 从结果集构建用户实体
FKUserEntity FKUserMapper::_buildUserFromRow(const mysqlx::Row& row) {
    int id = row[0].get<int>();
    std::string uuid = row[1].get<std::string>();
    std::string username = row[2].get<std::string>();
    std::string email = row[3].get<std::string>();
    std::string password = row[4].get<std::string>();
    std::string salt = row[5].get<std::string>();
    
    // 处理时间字段
    auto createTimeStr = row[6].get<std::string>();
    std::tm createTm = {};
    std::istringstream createSs(createTimeStr);
    createSs >> std::get_time(&createTm, "%Y-%m-%d %H:%M:%S");
    auto createTime = std::chrono::system_clock::from_time_t(std::mktime(&createTm));
    
    std::optional<std::chrono::system_clock::time_point> updateTime;
    if (!row[7].isNull()) {
        auto updateTimeStr = row[7].get<std::string>();
        std::tm updateTm = {};
        std::istringstream updateSs(updateTimeStr);
        updateSs >> std::get_time(&updateTm, "%Y-%m-%d %H:%M:%S");
        updateTime = std::chrono::system_clock::from_time_t(std::mktime(&updateTm));
    }
    
    return FKUserEntity(id, uuid, username, email, password, salt, createTime, updateTime);
}