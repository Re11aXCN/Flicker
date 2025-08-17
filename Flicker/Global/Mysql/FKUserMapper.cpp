#include "FKUserMapper.h"

#include "universal/mysql/time.h"
using namespace universal::mysql;
FKUserMapper::FKUserMapper(ConnectionPool* connPool)
    : BaseMapper<FKUserEntity, std::uint32_t>(connPool)
{

}

constexpr std::string FKUserMapper::getTableName() const {
    return "users";
}

constexpr std::string FKUserMapper::createTableQuery() const
{
    return "CREATE TABLE IF NOT EXISTS "
        "users ( "
        "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
        "uuid CHAR(36) NOT NULL UNIQUE DEFAULT (UUID()), "
        "username VARCHAR(30) NOT NULL UNIQUE, "
        "email VARCHAR(320) NOT NULL UNIQUE, "
        "password CHAR(60) NOT NULL, "
        "create_time TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3), "
        "update_time TIMESTAMP(3) DEFAULT NULL, "
        "INDEX idx_email (email), "
        "INDEX idx_username (username) "
        ") "
        "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;";
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
        "FROM " + getTableName() + " WHERE BINARY username = ?";
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

bool FKUserMapper::bindInsertParams(StmtPtr& stmtPtr, const FKUserEntity& entity) const {
    MYSQL_STMT* stmt = stmtPtr.get();
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
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(entity.getPassword().c_str());
    bind[2].buffer_length = *passwordLength + 1;
    bind[2].length = passwordLength;
  
    if (mysql_stmt_bind_param(stmt, bind)) {
        LOGGER_ERROR(std::format("Bind param failed: {}", mysql_stmt_error(stmt)));
        return false;
    }
    return true;
}

FKUserEntity FKUserMapper::createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
    char* isNulls, size_t columnCount) const 
{
    using tp = std::chrono::system_clock::time_point;
    if (columnCount < 7) {
        throw std::runtime_error("Insufficient columns in result set");
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
        throw std::runtime_error("Insufficient columns in result set");
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
    auto results = queryEntities<>(findByEmailQuery(), mysql_varchar{ email.data(), static_cast<unsigned long>(email.length()) });

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    return results.value().front();
}

std::optional<FKUserEntity> FKUserMapper::findByUsername(const std::string& username) {
    auto results = queryEntities<>(findByUsernameQuery(), mysql_varchar{ username.data(), static_cast<unsigned long>(username.length()) });

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    return results.value().front();
}

MySQLResult<uint64_t> FKUserMapper::updatePasswordByEmail(const std::string& email, const std::string& password)
{
    std::string query = updatePasswordByEmailQuery();
    auto stmtPtrResult = this->prepareStatement(query);
    if (!stmtPtrResult) {
        return std::unexpected(stmtPtrResult.error());
    }
    StmtPtr stmtPtr = std::move(stmtPtrResult.value());

    auto bindResult = bindValues(stmtPtr,
        mysql_char{ password.data(), static_cast<unsigned long>(password.length()) },
        time::mysqlCurrentTime(),
        mysql_varchar{ email.data(), static_cast<unsigned long>(email.length()) }
    );
    if (!bindResult) {
        return std::unexpected(bindResult.error());
    }
    auto queryResult = executeQuery(stmtPtr);
    if (!queryResult) {
        return std::unexpected(queryResult.error());
    }

    return mysql_stmt_affected_rows(stmtPtr.get());
}

MySQLResult<uint64_t>  FKUserMapper::deleteByEmail(const std::string& email)
{
    std::string query = deleteByEmailQuery();
    auto stmtPtrResult = this->prepareStatement(query);
    if (!stmtPtrResult) {
        return std::unexpected(stmtPtrResult.error());
    }
    StmtPtr stmtPtr = std::move(stmtPtrResult.value());

    auto bindResult = bindValues(stmtPtr, mysql_varchar{ email.data(), static_cast<unsigned long>(email.length()) });
    if (!bindResult) {
        return std::unexpected(bindResult.error());
    }
    auto queryResult = executeQuery(stmtPtr);
    if (!queryResult) {
        return std::unexpected(queryResult.error());
    }

    return mysql_stmt_affected_rows(stmtPtr.get());
}

bool FKUserMapper::isUsernameExists(const std::string& username)
{
    std::string query = _isUsernameExistsQuery();
    auto stmtPtrResult = this->prepareStatement(query);
    if (!stmtPtrResult) {
        return false;
    }
    StmtPtr stmtPtr = std::move(stmtPtrResult.value());

    auto bindResult = bindValues(stmtPtr, mysql_varchar{ username.data(), static_cast<unsigned long>(username.length()) });
    if (!bindResult) {
        return false;
    }
    auto queryResult = executeQuery(stmtPtr);
    if (!queryResult) {
        return false;
    }

    if (mysql_stmt_store_result(stmtPtr.get())) {
        LOGGER_ERROR(std::format("Store result failed: {}", mysql_stmt_error(stmtPtr.get())));
        return false;
    }

    my_ulonglong row_count = mysql_stmt_num_rows(stmtPtr.get());
    return (row_count > 0);
}

bool FKUserMapper::isEmailExists(const std::string& email)
{
    std::string query = _isEmailExistsQuery();
    auto stmtPtrResult = this->prepareStatement(query);
    if (!stmtPtrResult) {
        return false;
    }
    StmtPtr stmtPtr = std::move(stmtPtrResult.value());

    auto bindResult = bindValues(stmtPtr, mysql_varchar{ email.data(), static_cast<unsigned long>(email.length()) });
    if (!bindResult) {
        return false;
    }
    auto queryResult = executeQuery(stmtPtr);
    if (!queryResult) {
        return false;
    }

    if (mysql_stmt_store_result(stmtPtr.get())) {
        LOGGER_ERROR(std::format("Store result failed: {}", mysql_stmt_error(stmtPtr.get())));
        return false;
    }

    my_ulonglong row_count = mysql_stmt_num_rows(stmtPtr.get());
    return (row_count > 0);
}

std::optional<std::string> FKUserMapper::findUuidByEmail(const std::string& email)
{
    std::vector<std::string> fields{ "uuid" };
    auto bindCondition = QueryConditionBuilder::eq_("email", mysql_varchar{ email.data(), (unsigned long)email.length() });
    auto results = queryFieldsByCondition<>(bindCondition, fields);

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    const auto& row = results.value().front();
    auto [uuidOpt, status] = getFieldMapValue<std::string>(row, "uuid");
    return uuidOpt;
}

std::optional<std::string> FKUserMapper::findUuidByUsername(const std::string& username)
{
    std::vector<std::string> fields = { "uuid" };
    auto bindCondition = QueryConditionBuilder::eq_("username", mysql_varchar{ username.data(), (unsigned long)username.length() });
    auto results = queryFieldsByCondition<>(bindCondition, fields);

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    const auto& row = results.value().front();
    auto [uuidOpt, status] = getFieldMapValue<std::string>(row, "uuid");
    return uuidOpt;
}

std::optional<std::string> FKUserMapper::findPasswordByEmail(const std::string& email)
{
    std::vector<std::string> fields{ "password" };
    auto bindCondition = QueryConditionBuilder::eq_("email", mysql_varchar{ email.data(), (unsigned long)email.length() });
    auto results = queryFieldsByCondition<>(bindCondition, fields);

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    const auto& row = results.value().front();
    auto [passwordOpt, status] = getFieldMapValue<std::string>(row, "password");
    return passwordOpt;
}

std::optional<std::string> FKUserMapper::findPasswordByUsername(const std::string& username)
{
    std::vector<std::string> fields = { "password" };
    auto bindCondition = QueryConditionBuilder::eq_("username", mysql_varchar{ username.data(), (unsigned long)username.length() });
    auto results = queryFieldsByCondition<>(bindCondition, fields);

    if (!results || results.value().empty()) {
        return std::nullopt;
    }

    const auto& row = results.value().front();
    auto [passwordOpt, status] = getFieldMapValue<std::string>(row, "password");
    return passwordOpt;
}
