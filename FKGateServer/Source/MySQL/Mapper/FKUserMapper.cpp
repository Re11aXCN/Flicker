#include "FKUserMapper.h"

#

FKUserMapper::FKUserMapper() : FKBaseMapper<FKUserEntity, std::uint32_t>() {

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

FKUserEntity FKUserMapper::createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
    char* isNulls, size_t columnCount) const 
{
    using tp = std::chrono::system_clock::time_point;
    if (columnCount < 7) {
        throw DatabaseException("Insufficient columns in result set");
    }
    return FKUserEntity{
        _parser.getValue<std::uint32_t>(&binds[0], lengths[0], isNulls[0], fields[0].type).value(),
        _parser.getValue<std::string>(&binds[1], lengths[1], isNulls[1], fields[1].type).value(),
        _parser.getValue<std::string>(&binds[2], lengths[2], isNulls[2], fields[2].type).value(),
        _parser.getValue<std::string>(&binds[3], lengths[3], isNulls[3], fields[3].type).value(),
        _parser.getValue<std::string>(&binds[4], lengths[4], isNulls[4], fields[4].type).value(),
        _parser.getValue<tp>(&binds[5], lengths[5], isNulls[5], fields[5].type).value(),
        _parser.getValue<tp>(&binds[6], lengths[6], isNulls[6], fields[6].type)
    };
}

FKUserEntity FKUserMapper::createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields,
    unsigned long* lengths, size_t columnCount) const
{
    using tp = std::chrono::system_clock::time_point;
    if (columnCount < 7) {
        throw DatabaseException("Insufficient columns in result set");
    }
    return FKUserEntity{
        _parser.getValue<std::uint32_t>(row, 0, lengths, false, fields[0].type).value(),
        _parser.getValue<std::string>(row, 1, lengths, false, fields[1].type).value(),
        _parser.getValue<std::string>(row, 2, lengths, false, fields[2].type).value(),
        _parser.getValue<std::string>(row, 3, lengths, false, fields[3].type).value(),
        _parser.getValue<std::string>(row, 4, lengths, false, fields[4].type).value(),
        _parser.getValue<tp>(row, 5, lengths, false, fields[5].type).value(),
        _parser.getValue<tp>(row, 6, lengths, ResultSetParser::isFieldNull(row, 6), fields[6].type)
    };
}

std::optional<FKUserEntity> FKUserMapper::findByEmail(const std::string& email) {
    try {
        auto results = queryEntities<>(findByEmailQuery(), varchar{ email.data(), static_cast<unsigned long>(email.length()) });
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        return results.front();
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
        
        return results.front();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByUsername error: {}", e.what()));
        throw;
    }
}

DbOperator::Status FKUserMapper::updatePasswordByEmail(const std::string& email, const std::string& password) 
{
    try {
        std::string query = updatePasswordByEmailQuery();
        MySQLStmtPtr stmtPtr = prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for updatePasswordByEmail");
        }

        bindValues(stmtPtr, 
            varchar{ password.data(), static_cast<unsigned long>(password.length()) },
            MysqlTimeUtils::mysqlCurrentTime(),
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
        std::string query = deleteByEmailQuery();
        MySQLStmtPtr stmtPtr = prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) {
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
        std::string query = _isUsernameExistsQuery();
        MySQLStmtPtr stmtPtr = prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) {
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
        std::string query = _isEmailExistsQuery();
        MySQLStmtPtr stmtPtr = prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) {
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
        std::vector<std::string> fields{"password"};
        auto bindCondition = QueryConditionBuilder::eq_("email", varchar{ email.data(), (unsigned long)email.length() });
        auto results = queryFieldsByCondition<>(bindCondition, fields);
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        const auto& row = results.front();
        auto [passwordOpt, status] = getFieldMapValue<std::string>(row, "password");
        return passwordOpt;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByEmail error: {}", e.what()));
        throw;
    }
}

std::optional<std::string> FKUserMapper::findPasswordByUsername(const std::string& username)
{
    try {
        std::vector<std::string> fields = { "password" };
        auto bindCondition = QueryConditionBuilder::eq_("username", varchar{ username.data(), (unsigned long)username.length() });
        auto results = queryFieldsByCondition<>(bindCondition, fields);
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        const auto& row = results.front();
        auto [passwordOpt, status] = getFieldMapValue<std::string>(row, "password");
        return passwordOpt;
    }
    catch (const DatabaseException& e) {
        FK_SERVER_ERROR(std::format("findPasswordByUsername error: {}", e.what()));
        throw;
    }
}
