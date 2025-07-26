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
    bind[2].buffer_length = *passwordLength + 1;
    bind[2].length = passwordLength;
  
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
        id = std::atoll(std::string_view(row[0], lengths[0]).data()); //FKUtils::load_le32(row[0]);
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
        createTimeMs = (uint64_t)std::atof(std::string_view(row[5], lengths[5]).data()); //FKUtils::load_le64(row[5]);
    }

    uint64_t updateTimeMs = 0;
    if (row[6]) {
        updateTimeMs = (uint64_t)std::atof(std::string_view(row[6], lengths[6]).data());
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
        auto results = queryEntities<>(findByEmailQuery(), varchar{ email.data(), static_cast<unsigned long>(email.length()) });
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        return results[0];
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByEmail error: {}", e.what()));
        throw;
    }
}

std::optional<FKUserEntity> FKUserMapper::findByUsername(const std::string& username) {
    try {
        auto results = queryEntities<>(findByUsernameQuery(), varchar{ username.data(), static_cast<unsigned long>(username.length()) });
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        return results[0];
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

        bindValues(stmtPtr, 
            varchar{ password.data(), static_cast<unsigned long>(password.length()) },
            mysqlCurrentTime(),
            varchar{ email.data(), static_cast<unsigned long>(email.length()) }
        );
        executeQuery(stmtPtr);
        
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
        bindValues(stmtPtr, varchar{ email.data(), static_cast<unsigned long>(email.length()) });
        executeQuery(stmtPtr);
        
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

        bindValues(stmtPtr, varchar{ username.data(), static_cast<unsigned long>(username.length()) });
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

        bindValues(stmtPtr, varchar{ email.data(), static_cast<unsigned long>(email.length()) });
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
        std::vector<std::string> fields = {"password"};
        std::vector<std::string> values = {email};
        auto results = queryFieldsByCondition<>(
            getTableName(),
            fields,
            "email = ?",
            values
        );
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        const auto& row = results[0];
        auto it = row.find("password");
        if (it != row.end() && !it->second.empty()) {
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
        std::vector<std::string> fields = {"password"};
        std::vector<std::string> values = {username};

        auto results = queryFieldsByCondition<>(
            getTableName(),
            fields,
            "username = ?",
            values
        );
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        const auto& row = results[0];
        auto it = row.find("password");
        if (it != row.end() && !it->second.empty()) {
            return it->second;
        }
        
        return std::nullopt;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByUsername error: {}", e.what()));
        throw;
    }
}
