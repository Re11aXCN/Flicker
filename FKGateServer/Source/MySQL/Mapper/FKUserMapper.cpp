#include "FKUserMapper.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "FKLogger.h"

FKUserMapper::FKUserMapper() : FKBaseMapper<FKUserEntity, std::size_t>() {

}

constexpr std::string FKUserMapper::getTableName() const {
    return "users";
}

constexpr std::string FKUserMapper::findByIdQuery() const {
    return "SELECT id, uuid, username, email, password, "
           "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
           "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
           "FROM " + getTableName() + " WHERE id = ?";
}

constexpr std::string FKUserMapper::findAllQuery() const {
    return "SELECT id, uuid, username, email, password, "
           "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
           "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
           "FROM " + getTableName() + " ORDER BY id";
}

constexpr std::string FKUserMapper::findByEmailQuery() const {
    return "SELECT id, uuid, username, email, password, "
        "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
        "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
        "FROM " + getTableName() + " WHERE email = ?";
}

constexpr std::string FKUserMapper::findByUsernameQuery() const {
    return "SELECT id, uuid, username, email, password, "
        "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
        "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
        "FROM " + getTableName() + " WHERE username = ?";
}

constexpr std::string FKUserMapper::insertQuery() const {
    return "INSERT INTO " + getTableName() + 
           " (uuid, username, email, password, create_time) "
           "VALUES (?, ?, ?, ?, ?)";
}

constexpr std::string FKUserMapper::deleteByIdQuery() const {
    return "DELETE FROM " + getTableName() + " WHERE id = ?";
}

constexpr std::string FKUserMapper::updatePasswordByEmailQuery() const {
    return "UPDATE " + getTableName() + 
           " SET password = ?, update_time = ? WHERE email = ?";
}

constexpr std::string FKUserMapper::deleteByEmailQuery() const {
    return "DELETE FROM " + getTableName() + " WHERE email = ?";
}

constexpr std::string FKUserMapper::_isUsernameExistsQuery() const
{
    return "SELECT 1 FROM " + getTableName() + " WHERE username = ? LIMIT 1";
}


constexpr std::string FKUserMapper::_isEmailExistsQuery() const
{
    return "SELECT 1 FROM " + getTableName() + " WHERE email = ? LIMIT 1";
}


constexpr std::string FKUserMapper::_findPasswordByEmailQuery()
{
    return "SELECT password FROM " + getTableName() + " WHERE email = ?";
}


constexpr std::string FKUserMapper::_findPasswordByUsernameQuery()
{
    return "SELECT password FROM " + getTableName() + " WHERE username = ?";
}

void FKUserMapper::bindIdParam(MYSQL_STMT* stmt, std::size_t id) const {
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = const_cast<std::size_t*>(&id);
    bind[0].is_unsigned = true;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindInsertParams(MYSQL_STMT* stmt, const FKUserEntity& entity) const {
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    
    // UUID
    std::string uuid = entity.getUuid();
    unsigned long uuidLength = uuid.length();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(uuid.c_str());
    bind[0].buffer_length = uuidLength;
    bind[0].length = &uuidLength;
    
    // Username
    std::string username = entity.getUsername();
    unsigned long usernameLength = username.length();
    
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(username.c_str());
    bind[1].buffer_length = usernameLength;
    bind[1].length = &usernameLength;
    
    // Email
    std::string email = entity.getEmail();
    unsigned long emailLength = email.length();
    
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(email.c_str());
    bind[2].buffer_length = emailLength;
    bind[2].length = &emailLength;
    
    // Password
    std::string password = entity.getPassword();
    unsigned long passwordLength = password.length();
    
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = const_cast<char*>(password.c_str());
    bind[3].buffer_length = passwordLength;
    bind[3].length = &passwordLength;
    
//     // Create Time
//     MYSQL_TIME createTime;
//     auto createTimePoint = entity.getCreateTime();
//     auto createTimeT = std::chrono::system_clock::to_time_t(createTimePoint);
    
//     // 线程安全的时间转换
// #if defined(_WIN32) || defined(_WIN64)
//     struct tm timeinfo;
//     gmtime_s(&timeinfo, &createTimeT);
// #else
//     // Linux/macOS使用gmtime_r
//     struct tm timeinfo;
//     gmtime_r(&createTimeT, &timeinfo);
// #endif

//     auto duration = createTimePoint.time_since_epoch();
//     auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

//     memset(&createTime, 0, sizeof(createTime));
//     createTime.year = timeinfo.tm_year + 1900;
//     createTime.month = timeinfo.tm_mon + 1;
//     createTime.day = timeinfo.tm_mday;
//     createTime.hour = timeinfo.tm_hour;
//     createTime.minute = timeinfo.tm_min;
//     createTime.second = timeinfo.tm_sec;
//     createTime.second_part = millis * 1000;  // 毫秒转微秒
//     createTime.time_type = MYSQL_TIMESTAMP_DATETIME;

//     bind[4].buffer_type = MYSQL_TYPE_TIMESTAMP;
//     bind[4].buffer = &createTime;
//     bind[4].is_null = 0;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindEmailParam(MYSQL_STMT* stmt, const std::string& email) const {
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    unsigned long emailLength = email.length();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(email.c_str());
    bind[0].buffer_length = emailLength;
    bind[0].length = &emailLength;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindUsernameParam(MYSQL_STMT* stmt, const std::string& username) const {
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    unsigned long usernameLength = username.length();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = usernameLength;
    bind[0].length = &usernameLength;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindPasswordAndEmailParams(MYSQL_STMT* stmt, 
                                             const std::string& password, 
                                             const std::string& email) const {
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    
    // Password
    unsigned long passwordLength = password.length();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(password.c_str());
    bind[0].buffer_length = passwordLength;
    bind[0].length = &passwordLength;

    // Update Time (current UTC time with milliseconds)
    MYSQL_TIME updateTime;
    memset(&updateTime, 0, sizeof(updateTime));

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    // 线程安全的时间转换
#if defined(_WIN32) || defined(_WIN64)
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_time_t);
#else
    // Linux/macOS使用localtime_r
    struct tm timeinfo;
    localtime_r(&now_time_t, &timeinfo);
#endif
    updateTime.year = timeinfo.tm_year + 1900;
    updateTime.month = timeinfo.tm_mon + 1;
    updateTime.day = timeinfo.tm_mday;
    updateTime.hour = timeinfo.tm_hour;
    updateTime.minute = timeinfo.tm_min;
    updateTime.second = timeinfo.tm_sec;
    updateTime.second_part = millis * 1000;  // 毫秒转微秒
    updateTime.time_type = MYSQL_TIMESTAMP_DATETIME;

    bind[1].buffer_type = MYSQL_TYPE_TIMESTAMP;
    bind[1].buffer = &updateTime;
    bind[1].is_null = 0;

    // Email
    unsigned long emailLength = email.length();
    
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(email.c_str());
    bind[2].buffer_length = emailLength;
    bind[2].length = &emailLength;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

FKUserEntity FKUserMapper::createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const {
    if (!row) {
        throw DatabaseException("Invalid row data");
    }
    
    // 解析ID
    std::size_t id = 0;
    if (row[0]) {
        id = std::stoull(std::string(row[0], lengths[0]));
    }
    
    // 解析UUID
    std::string uuid;
    if (row[1]) {
        uuid = std::string(row[1], lengths[1]);
    }
    
    // 解析用户名
    std::string username;
    if (row[2]) {
        username = std::string(row[2], lengths[2]);
    }
    
    // 解析邮箱
    std::string email;
    if (row[3]) {
        email = std::string(row[3], lengths[3]);
    }
    
    // 解析密码
    std::string password;
    if (row[4]) {
        password = std::string(row[4], lengths[4]);
    }
    
    // 解析创建时间
    std::chrono::system_clock::time_point createTime;
    if (row[5]) {
        uint64_t ms = std::stoull(std::string(row[5], lengths[5]));
        createTime = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
    }

    // 解析更新时间
    std::optional<std::chrono::system_clock::time_point> updateTime = std::nullopt;
    if (row[6]) {
        uint64_t ms = std::stoull(std::string(row[6], lengths[6]));
        updateTime = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
    }

    return FKUserEntity(id, uuid, username, email, password, createTime, updateTime);
}

std::optional<FKUserEntity> FKUserMapper::findByEmail(const std::string& email) {
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(findByEmailQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for findByEmail");
        }
        
        // 绑定参数
        bindEmailParam(stmt, email);
        
        // 执行查询
        executeQuery(stmt);
        
        // 获取元数据
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for findByEmail query");
        }
        
        // 获取结果
        std::optional<FKUserEntity> result;
        try {
            result = fetchSingleResult(stmt, meta);
        } catch (const DatabaseException& e) {
            // 如果没有找到结果，返回空optional
            if (std::string(e.what()).find("No results found") != std::string::npos) {
                result = std::nullopt;
            } else {
                throw; // 重新抛出其他异常
            }
        }
        
        // 清理资源
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        
        return result;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByEmail error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}

std::optional<FKUserEntity> FKUserMapper::findByUsername(const std::string& username) {
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(findByUsernameQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for findByUsername");
        }
        
        // 绑定参数
        bindUsernameParam(stmt, username);
        
        // 执行查询
        executeQuery(stmt);
        
        // 获取元数据
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for findByUsername query");
        }
        
        // 获取结果
        std::optional<FKUserEntity> result;
        try {
            result = fetchSingleResult(stmt, meta);
        } catch (const DatabaseException& e) {
            // 如果没有找到结果，返回空optional
            if (std::string(e.what()).find("No results found") != std::string::npos) {
                result = std::nullopt;
            } else {
                throw; // 重新抛出其他异常
            }
        }
        
        // 清理资源
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        
        return result;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByUsername error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}

DbOperator::Status FKUserMapper::updatePasswordByEmail(const std::string& email, const std::string& password) 
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(updatePasswordByEmailQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for updatePasswordByEmail");
        }
        
        // 绑定参数
        bindPasswordAndEmailParams(stmt, password, email);
        
        // 执行更新
        executeUpdate(stmt);
        
        // 检查影响的行数
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmt);
        
        // 清理资源
        mysql_stmt_close(stmt);
        
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        
        return DbOperator::Status::Success;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updatePasswordByEmail error: {}", e.what()));
        mysql_stmt_close(stmt);
        return DbOperator::Status::DatabaseError;
    }
}

DbOperator::Status FKUserMapper::deleteByEmail(const std::string& email) 
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(deleteByEmailQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for deleteByEmail");
        }
        
        // 绑定参数
        bindEmailParam(stmt, email);
        
        // 执行更新
        executeUpdate(stmt);
        
        // 检查影响的行数
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmt);
        
        // 清理资源
        mysql_stmt_close(stmt);
        
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        
        return DbOperator::Status::Success;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("deleteByEmail error: {}", e.what()));
        mysql_stmt_close(stmt);
        return DbOperator::Status::DatabaseError;
    }
}

bool FKUserMapper::isUsernameExists(const std::string& username)
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(_isUsernameExistsQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for isUsernameExists");
        }

        bindUsernameParam(stmt, username);
        executeQuery(stmt);
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for isUsernameExists query");
        }
        bool result = false;
        MYSQL_ROW row = mysql_fetch_row(meta);
        if (row) {
            result = true;
        }
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return result;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("isUsernameExists error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}

bool FKUserMapper::isEmailExists(const std::string& email)
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(_isEmailExistsQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for isEmailExists");
        }

        bindEmailParam(stmt, email);
        executeQuery(stmt);
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for isEmailExists query");
        }
        bool result = false;
        MYSQL_ROW row = mysql_fetch_row(meta);
        if (row) {
            result = true;
        }
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return result;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("isEmailExists error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}

std::optional<std::string> FKUserMapper::findPasswordByEmail(const std::string& email)
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(_findPasswordByEmailQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for findPasswordByEmail");
        }

        bindEmailParam(stmt, email);
        executeQuery(stmt);
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for findPasswordByEmail query");
        }
        std::optional<std::string> result;
        MYSQL_ROW row = mysql_fetch_row(meta);
        if (row) {
            result = std::string(row[0], mysql_fetch_lengths(meta)[0]);
        }
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return result;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByEmail error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}

std::optional<std::string> FKUserMapper::findPasswordByUsername(const std::string& username)
{
    MYSQL_STMT* stmt = nullptr;
    try {
        stmt = prepareStatement(_findPasswordByUsernameQuery().c_str());
        if (!stmt) {
            throw DatabaseException("Failed to prepare statement for findPasswordByUsername");
        }

        bindUsernameParam(stmt, username);
        executeQuery(stmt);
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            mysql_stmt_close(stmt);
            throw DatabaseException("No metadata available for findPasswordByUsername query");
        }
        std::optional<std::string> result;
        MYSQL_ROW row = mysql_fetch_row(meta);
        if (row) {
            result = std::string(row[0], mysql_fetch_lengths(meta)[0]);
        }
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return result;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByUsername error: {}", e.what()));
        mysql_stmt_close(stmt);
        throw;
    }
}
