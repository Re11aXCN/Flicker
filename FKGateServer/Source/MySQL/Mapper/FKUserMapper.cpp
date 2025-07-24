#include "FKUserMapper.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "FKLogger.h"

struct MetaGuard {
    MYSQL_RES* res;
    MetaGuard(MYSQL_RES* r) : res(r) {}
    ~MetaGuard() { if (res) mysql_free_result(res); }
};

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
           " (username, email, password) "
           "VALUES (?, ?, ?)";
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

void FKUserMapper::bindIdParam(MySQLStmtPtr& stmtPtr, std::size_t* id) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = const_cast<std::size_t*>(id);
    bind[0].is_unsigned = true;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindInsertParams(MySQLStmtPtr& stmtPtr, const FKUserEntity& entity) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));

    // Username
    auto* usernameLength = const_cast<unsigned long*>(entity.getUsernameLength());
    const std::string& username = entity.getUsername();
    bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = *usernameLength + 1;
    bind[0].length = usernameLength;
    
    // Email
    auto* emailLength = const_cast<unsigned long*>(entity.getEmailLength());
    const std::string& email = entity.getEmail();
    bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[1].buffer = const_cast<char*>(entity.getEmail().c_str());
    bind[1].buffer_length = *emailLength + 1;
    bind[1].length = emailLength;

    // Password
    auto* passwordLength = const_cast<unsigned long*>(entity.getPasswordLength());
    const std::string& password = entity.getPassword();;
    bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer = const_cast<char*>(entity.getPassword().c_str());
    bind[2].buffer_length = *passwordLength;
    bind[2].length = passwordLength;
    
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
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindEmailParam(MySQLStmtPtr& stmtPtr, const std::string& email, unsigned long* length) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[0].buffer = const_cast<char*>(email.c_str());
    bind[0].buffer_length = *length + 1;
    bind[0].length = length;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindUsernameParam(MySQLStmtPtr& stmtPtr, const std::string& username, unsigned long* length) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = *length + 1;
    bind[0].length = length;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

void FKUserMapper::bindPasswordAndEmailParams(MySQLStmtPtr& stmtPtr, 
                                             const std::string& password, unsigned long* passwordLength,
                                             const std::string& email, unsigned long* emailLength) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    
    // Password
    bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[0].buffer = const_cast<char*>(password.c_str());
    bind[0].buffer_length = *passwordLength + 1;
    bind[0].length = passwordLength;

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
    bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer = const_cast<char*>(email.c_str());
    bind[2].buffer_length = *emailLength + 1;
    bind[2].length = emailLength;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

FKUserEntity FKUserMapper::createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const {
    if (!row) {
        throw DatabaseException("Invalid row data");
    }

    std::size_t id = 0;
    if (row[0]) {
        id = FKUtils::load_le32(row[0]);
    }
    
    std::string uuid;
    if (row[1]) {
        uuid = std::string(row[1], lengths[1]);
    }
    
    std::string username;
    if (row[2]) {
        username = std::string(row[2], lengths[2]);
    }
    
    std::string email;
    if (row[3]) {
        email = std::string(row[3], lengths[3]);
    }
    
    std::string password;
    if (row[4]) {
        password = std::string(row[4], lengths[4]);
    }
    
    std::chrono::system_clock::time_point createTime;
    uint64_t createTimeMs = 0;
    if (row[5]) {
        /*uint64_t milliSeconds;
        std::memcpy(&milliSeconds, row[5], sizeof(uint64_t));
        milliSeconds = (Swap64(milliSeconds));*/
        createTimeMs = FKUtils::load_le64(row[5]);
    }

    uint64_t updateTimeMs = 0;
    if (row[6]) {
        updateTimeMs = FKUtils::load_le64(row[5]);
    }
    
    using sc = std::chrono::system_clock;
    return FKUserEntity(id, std::move(uuid), std::move(username), std::move(email), std::move(password),
        std::move(sc::time_point{std::chrono::milliseconds{createTimeMs}}),
        std::move(updateTimeMs == 0 
            ? std::optional<sc::time_point>{}
            : std::optional<sc::time_point>{ sc::time_point{std::chrono::milliseconds{updateTimeMs}} })
    );
}

std::optional<FKUserEntity> FKUserMapper::findByEmail(const std::string& email) {
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(findByEmailQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for findByEmail");
        }
        unsigned long emailLength = static_cast<unsigned long>(email.length());
        bindEmailParam(stmtPtr, email, &emailLength);

        executeQuery(stmtPtr);
        
        // 获取元数据并使用RAII守卫确保资源释放
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmtPtr);
        if (!meta) {
            throw DatabaseException("No metadata available for findByEmail query");
        }
        
        // 创建RAII守卫确保meta资源释放
        MetaGuard metaGuard(meta);
        
        return fetchSingleResult(stmtPtr, meta);
        
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByEmail error: {}", e.what()));
        throw;
    }
}

std::optional<FKUserEntity> FKUserMapper::findByUsername(const std::string& username) {
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(findByUsernameQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for findByUsername");
        }
        unsigned long usernameLength = static_cast<unsigned long>(username.length());
        bindUsernameParam(stmtPtr, username, &usernameLength);
        
        executeQuery(stmtPtr);
        
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmtPtr);
        if (!meta) {
            throw DatabaseException("No metadata available for findByUsername query");
        }
        
        MetaGuard metaGuard(meta);
        
        return fetchSingleResult(stmtPtr, meta);
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByUsername error: {}", e.what()));
        throw;
    }
}

DbOperator::Status FKUserMapper::updatePasswordByEmail(const std::string& email, const std::string& password) 
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(updatePasswordByEmailQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for updatePasswordByEmail");
        }
        unsigned long passwordLength = static_cast<unsigned long>(password.length());
        unsigned long emailLength = static_cast<unsigned long>(email.length());
        bindPasswordAndEmailParams(stmtPtr, password, &passwordLength, email, &emailLength);

        executeUpdate(stmtPtr);
        
        // 检查影响的行数
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        
        return DbOperator::Status::Success;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updatePasswordByEmail error: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

DbOperator::Status FKUserMapper::deleteByEmail(const std::string& email) 
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(deleteByEmailQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for deleteByEmail");
        }
        unsigned long emailLength = static_cast<unsigned long>(email.length());
        bindEmailParam(stmtPtr, email, &emailLength);
        
        executeUpdate(stmtPtr);
        
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        
        return DbOperator::Status::Success;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("deleteByEmail error: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

bool FKUserMapper::isUsernameExists(const std::string& username)
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(_isUsernameExistsQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for isUsernameExists");
        }

        unsigned long usernameLength = static_cast<unsigned long>(username.length());
        bindUsernameParam(stmtPtr, username, &usernameLength);
        executeQuery(stmtPtr);
        
        if (mysql_stmt_store_result(stmtPtr)) {
            std::string error = mysql_stmt_error(stmtPtr);
            throw DatabaseException("Store result failed: " + error);
        }

        my_ulonglong row_count = mysql_stmt_num_rows(stmtPtr);
        return (row_count > 0);
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("isUsernameExists error: {}", e.what()));
        throw;
    }
}

bool FKUserMapper::isEmailExists(const std::string& email)
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(_isEmailExistsQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for isEmailExists");
        }

        unsigned long emailLength = static_cast<unsigned long>(email.length());
        bindEmailParam(stmtPtr, email, &emailLength);
        executeQuery(stmtPtr);
        
        if (mysql_stmt_store_result(stmtPtr)) {
            std::string error = mysql_stmt_error(stmtPtr);
            throw DatabaseException("Store result failed: " + error);
        }

        my_ulonglong row_count = mysql_stmt_num_rows(stmtPtr);
        return (row_count > 0);
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("isEmailExists error: {}", e.what()));
        throw;
    }
}

std::optional<std::string> FKUserMapper::findPasswordByEmail(const std::string& email)
{
    try {
        // 使用fetchFieldsByCondition方法获取密码字段
        std::vector<std::string> fields = {"password"};
        auto results = fetchFieldsByCondition<FKUserEntity>(
            getTableName(),
            fields,
            "email",
            email
        );
        
        // 检查结果
        if (results.empty()) {
            return std::nullopt; // 未找到匹配的记录
        }
        
        // 返回密码
        const auto& row = results[0];
        auto it = row.find("password");
        if (it != row.end() && it->second != "NULL") {
            return it->second;
        }
        
        return std::nullopt;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByEmail error: {}", e.what()));
        throw;
    }
}

std::optional<std::string> FKUserMapper::findPasswordByUsername(const std::string& username)
{
    try {
        // 使用fetchFieldsByCondition方法获取密码字段
        std::vector<std::string> fields = {"password"};
        auto results = fetchFieldsByCondition<FKUserEntity>(
            getTableName(),
            fields,
            "username",
            username
        );
        
        // 检查结果
        if (results.empty()) {
            return std::nullopt; // 未找到匹配的记录
        }
        
        // 返回密码
        const auto& row = results[0];
        auto it = row.find("password");
        if (it != row.end() && it->second != "NULL") {
            return it->second;
        }
        
        return std::nullopt;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByUsername error: {}", e.what()));
        throw;
    }
}
