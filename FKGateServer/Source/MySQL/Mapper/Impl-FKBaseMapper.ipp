#define MSYSQL_QUERY_CONDITION_JUDGMENT(NeedBind)\
switch (condition->getConditionType()) {\
case ConditionType::TRUE_:\
case ConditionType::FALSE_: {\
    isNeedBind = false;\
    break;\
}\
case ConditionType::IN_:\
case ConditionType::NOTIN_:\
case ConditionType::AND_:\
case ConditionType::OR_: {\
    if (condition->buildConditionClause() == "1=0") {\
        FK_SERVER_WARN("IN / NOT IN / AND / OR condition must be set values " __FUNCTION__ "");\
        return {};\
    }\
    break;\
}\
default: break;\
}

#define MYSQL_FOREACH_BIND_RESSET(Fields, Binds, Lengths, IsNull, Buffers, ColumnCount)                                  \
for (std::size_t i = 0; i < ColumnCount; ++i) {                    \
    /*MYSQL_FIELD* field = mysql_fetch_field_direct(Res, i);*/     \
    const MYSQL_FIELD& field = Fields[i];                            \
    MYSQL_BIND& bind = Binds[i];                                   \
    bind.length = &Lengths[i];                                     \
    bind.is_null = reinterpret_cast<bool*>(&IsNull[i]);            \
                                                                     \
    switch (field.type) {                                            \
    case MYSQL_TYPE_TINY:                                            \
        bind.buffer_type = MYSQL_TYPE_TINY;                          \
        bind.buffer_length = sizeof(int8_t);                         \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        bind.is_unsigned = (field.flags & UNSIGNED_FLAG) ? 1 : 0;    \
        break;                                                       \
    case MYSQL_TYPE_SHORT:                                           \
        bind.buffer_type = MYSQL_TYPE_SHORT;                         \
        bind.buffer_length = sizeof(int16_t);                        \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        bind.is_unsigned = (field.flags & UNSIGNED_FLAG) ? 1 : 0;    \
        break;                                                       \
    case MYSQL_TYPE_INT24:                                           \
    case MYSQL_TYPE_LONG:                                            \
    case MYSQL_TYPE_YEAR:                                            \
        bind.buffer_type = MYSQL_TYPE_LONG;                          \
        bind.buffer_length = sizeof(int32_t);                        \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        bind.is_unsigned = (field.flags & UNSIGNED_FLAG) ? 1 : 0;    \
        break;                                                       \
    case MYSQL_TYPE_LONGLONG:                                        \
        bind.buffer_type = MYSQL_TYPE_LONGLONG;                      \
        bind.buffer_length = sizeof(int64_t);                        \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        bind.is_unsigned = (field.flags & UNSIGNED_FLAG) ? 1 : 0;    \
        break;                                                       \
    case MYSQL_TYPE_FLOAT:                                           \
        bind.buffer_type = MYSQL_TYPE_FLOAT;                         \
        bind.buffer_length = sizeof(float);                          \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        break;                                                       \
    case MYSQL_TYPE_DOUBLE:                                          \
    case MYSQL_TYPE_DECIMAL:                                         \
    case MYSQL_TYPE_NEWDECIMAL:                                      \
        bind.buffer_type = MYSQL_TYPE_DOUBLE;                        \
        bind.buffer_length = sizeof(double);                         \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        break;                                                       \
    case MYSQL_TYPE_TIMESTAMP:                                       \
    case MYSQL_TYPE_DATETIME:                                        \
        bind.buffer_type = field.type;                               \
        bind.buffer_length = sizeof(MYSQL_TIME);                     \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        break;                                                       \
    case MYSQL_TYPE_DATE:                                            \
    case MYSQL_TYPE_TIME:                                            \
    case MYSQL_TYPE_TINY_BLOB:                                       \
    case MYSQL_TYPE_MEDIUM_BLOB:                                     \
    case MYSQL_TYPE_LONG_BLOB:                                       \
    case MYSQL_TYPE_BLOB:                                            \
    case MYSQL_TYPE_VAR_STRING:                                      \
    case MYSQL_TYPE_STRING:                                          \
        bind.buffer_type = field.type;                               \
        bind.buffer_length = field.length > 0 ? field.length : 1024; \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        break;                                                       \
    default:                                                         \
        bind.buffer_type = MYSQL_TYPE_STRING;                        \
        bind.buffer_length = 1024;                                   \
        Buffers[i].resize(bind.buffer_length);                     \
        bind.buffer = Buffers[i].data();                           \
        break;                                                       \
    }                                                                \
}

template<typename Entity, typename ID_TYPE>
inline const auto& FKBaseMapper<Entity, ID_TYPE>::getFieldMappings() const
{
    return _fieldMappings;
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline std::optional<ValueType> FKBaseMapper<Entity, ID_TYPE>::getFieldValue(const std::string& fieldName) const
{
    if (auto it = _fieldMappings.find(fieldName); it != _fieldMappings.end()) {
        return std::get<ValueType>(it->second);
    }
    return std::nullopt;
}

template<typename Entity, typename ID_TYPE>
inline std::optional<Entity> FKBaseMapper<Entity, ID_TYPE>::findById(ID_TYPE id)
{
    auto results = this->queryEntities<>(findByIdQuery(), id);

    if (results.empty()) {
        return std::nullopt;
    }

    return results.front();
}

template<typename Entity, typename ID_TYPE>
template <SortOrder Order, fixed_string FieldName, Pagination Pagination>
inline FKBaseMapper<Entity, ID_TYPE>::EntityRows FKBaseMapper<Entity, ID_TYPE>::findAll()
{
    std::string query = FKUtils::concat(findAllQuery(),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>());
    this->_find_all_impl(query);
}

template<typename Entity, typename ID_TYPE>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::insert(const Entity& entity)
{
    try {
        std::string query = insertQuery();
        MySQLStmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for insert");
        }
        this->bindInsertParams(stmtPtr, entity);
        this->executeQuery(stmtPtr);
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return { DbOperator::Status::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return { DbOperator::Status::Success, affectedRows };
    }
    catch (const std::exception& e) {
        std::string error = e.what();
        FK_SERVER_ERROR(std::format("insert error: {}", error));
        if (error.find("Duplicate entry") != std::string::npos) {
            return { DbOperator::Status::DataAlreadyExist, 0 };
        }
        return { DbOperator::Status::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::deleteById(ID_TYPE id)
{
    try {
        std::string query = deleteByIdQuery();
        MySQLStmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");

        this->bindValues(stmtPtr, id);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return { DbOperator::Status::DataNotExist, 0};
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return { DbOperator::Status::Success, affectedRows };
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("deleteById error: {}", e.what()));
        return { DbOperator::Status::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ...Args>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::updateFieldsById(
     ID_TYPE id,
    const std::vector<std::string>& fieldNames, Args && ...args)
{
    if (fieldNames.empty()) {
        FK_SERVER_ERROR("Update fields is null in " __FUNCTION__ ", Cannot update without fields");
        return { DbOperator::Status::InvalidParameters, 0 };
    }
    try {
        // 参数数量检查
        if (sizeof...(args) != fieldNames.size()) {
            throw DatabaseException("Number of field names must match number of values");
        }

        std::string query = fieldNames.size() > 1
            ? FKUtils::concat(
                "UPDATE ", this->getTableName(),
                " SET ", FKUtils::join_range(" = ?, ", fieldNames),
                " = ? WHERE  id = ?")
            : FKUtils::concat(
                "UPDATE ", this->getTableName(),
                " SET ", fieldNames[0],
                " = ? WHERE  id = ?");
        MySQLStmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");

        this->bindValues(stmtPtr, std::forward<Args>(args)..., id);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return { DbOperator::Status::DataNotExist, 0 };

        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }
        return { DbOperator::Status::Success, affectedRows };
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updateFieldsByIdSafe error: {}", e.what()));
        return { DbOperator::Status::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName, Pagination Pagination,
    typename... Args >
inline FKBaseMapper<Entity, ID_TYPE>::EntityRows FKBaseMapper<Entity, ID_TYPE>::queryEntities(std::string&& sql, Args && ...args) const
{
    // 构建sql初始化
    std::string query = FKUtils::concat(std::move(sql),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>());
    MySQLStmtPtr stmtPtr = this->prepareStatement(query);

    if (!stmtPtr && !stmtPtr.isValid())  throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
    // 绑定条件参数并执行查询
    this->bindValues(stmtPtr, std::forward<Args>(args)...);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
    if (!resultGuard.res)  throw DatabaseException("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resultGuard.res);
    MYSQL_FIELD* fields = mysql_fetch_fields(resultGuard.res);

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw DatabaseException("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw DatabaseException("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }
    my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
    if (rowCount == 0) return {};

    EntityRows results;
    results.reserve(rowCount);
    while (true) {
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result == MYSQL_NO_DATA) {
            break;
        }
        else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
            throw DatabaseException("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
        }
        results.push_back(createEntityFromBinds(binds.get(), fields, lengths.get(),
            isNull.get(), columnCount));
    }
    return results;
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName, Pagination Pagination>
inline FKBaseMapper<Entity, ID_TYPE>::EntityRows FKBaseMapper<Entity, ID_TYPE>::queryEntitiesByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        FK_SERVER_WARN("Condition is null in " __FUNCTION__ "");
        return {};
    }
    std::string query = FKUtils::concat(
        "SELECT * FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause(),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>()
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    if (!isNeedBind) {
        return this->_find_all_impl(query);
    }
    
    MySQLStmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr && !stmtPtr.isValid())  throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
    this->bindConditionValues(stmtPtr, condition);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
    if (!resultGuard.res)  throw DatabaseException("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resultGuard.res);
    MYSQL_FIELD* fields = mysql_fetch_fields(resultGuard.res);

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw DatabaseException("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw DatabaseException("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }
    my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
    if (rowCount == 0) return {};

    EntityRows results;
    results.reserve(rowCount);
    while (true) {
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result == MYSQL_NO_DATA) {
            break;
        }
        else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
            throw DatabaseException("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
        }
        results.push_back(createEntityFromBinds(binds.get(), fields, lengths.get(),
            isNull.get(), columnCount));
    }
    return results;
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName, Pagination Pagination>
inline FKBaseMapper<Entity, ID_TYPE>::FieldMapRows FKBaseMapper<Entity, ID_TYPE>::queryFieldsByCondition(
    const std::unique_ptr<IQueryCondition>& condition, 
    const std::vector<std::string>& fieldNames) const
{
    if (!condition) {
        FK_SERVER_WARN("Condition is null in " __FUNCTION__ "");
        return {};
    }
    
    if (fieldNames.empty()) {
        FK_SERVER_WARN("Field names is empty in " __FUNCTION__ "");
        return {};
    }
    
    // 构建查询语句
    std::string query = FKUtils::concat(
        "SELECT ", FKUtils::join_range(", ", fieldNames),
        " FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause(),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>()
    );
    
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    if (!isNeedBind) {
        return this->_query_fields_impl(query);
    }

    MySQLStmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement in " __FUNCTION__ "");
    this->bindConditionValues(stmtPtr, condition);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
    if (!resultGuard.res)  throw DatabaseException("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resultGuard.res);
    MYSQL_FIELD* fields = mysql_fetch_fields(resultGuard.res);

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw DatabaseException("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw DatabaseException("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }
    my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
    if (rowCount == 0) return {};

    FieldMapRows results;
    results.reserve(rowCount);
    while (true) {
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result == MYSQL_NO_DATA) {
            break;
        }
        else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
            throw DatabaseException("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
        }
        FieldMap fieldMap;
        for (unsigned int i = 0; i < columnCount; ++i) {
            const MYSQL_FIELD& field = fields[i];
            const std::string fieldName = field.name;
            switch (field.type) {
            case MYSQL_TYPE_TINY:
                fieldMap[fieldName] = (field.flags & UNSIGNED_FLAG)
                    ? std::any(_parser.getValue<std::uint8_t>(&binds[i], lengths[i], isNull[i], field.type).value())
                    : std::any(_parser.getValue<std::int8_t>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_SHORT:
                fieldMap[fieldName] = (field.flags & UNSIGNED_FLAG)
                    ? std::any(_parser.getValue<std::uint16_t>(&binds[i], lengths[i], isNull[i], field.type).value())
                    : std::any(_parser.getValue<std::int16_t>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_YEAR:
                fieldMap[fieldName] = (field.flags & UNSIGNED_FLAG)
                    ? std::any(_parser.getValue<std::uint32_t>(&binds[i], lengths[i], isNull[i], field.type).value())
                    : std::any(_parser.getValue<std::int32_t>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_LONGLONG:
                fieldMap[fieldName] = (field.flags & UNSIGNED_FLAG)
                    ? std::any(_parser.getValue<std::uint64_t>(&binds[i], lengths[i], isNull[i], field.type).value())
                    : std::any(_parser.getValue<std::int64_t>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_FLOAT:
                fieldMap[fieldName] = std::any(_parser.getValue<float>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                fieldMap[fieldName] = std::any(_parser.getValue<double>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_DATETIME:
                fieldMap[fieldName] = std::any(_parser.getValue
                    <std::chrono::system_clock::time_point>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
                fieldMap[fieldName] = std::any(_parser.getValue<std::string>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            default:
                fieldMap[fieldName] = std::any(_parser.getValue<std::string>(&binds[i], lengths[i], isNull[i], field.type).value());
                break;
            }
        }

        results.push_back(std::move(fieldMap));
    }

    return results;
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline auto FKBaseMapper<Entity, ID_TYPE>::getFieldMapValue(const FieldMap& fieldMap, const std::string& fieldName) const
-> std::pair<std::optional<ValueType>, GetFieldMapResStatus>
{
    auto it = fieldMap.find(fieldName);
    if (it == fieldMap.end()) {
        return { std::nullopt, GetFieldMapResStatus::NotFound };
    }

    try {
        return { std::any_cast<ValueType>(it->second), GetFieldMapResStatus::Success };
    }
    catch (const std::bad_any_cast&) {
        FK_SERVER_ERROR("Type mismatch for key: " + fieldName);
        return { std::nullopt, GetFieldMapResStatus::BadCast };
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline ValueType FKBaseMapper<Entity, ID_TYPE>::getFieldMapValueOrDefault(const FieldMap& fieldMap,
    const std::string& fieldName, ValueType defaultValue) const
{
    auto [value, status] = this->getFieldMapValue<ValueType>(fieldMap, fieldName);
    return value.value_or(std::move(defaultValue));
}

template<typename Entity, typename ID_TYPE>
inline size_t FKBaseMapper<Entity, ID_TYPE>::countByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        FK_SERVER_WARN("Condition is null in " __FUNCTION__ "");
        return 0;
    }
    std::string query = FKUtils::concat(
        "SELECT COUNT(*) FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause()
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    if (!isNeedBind) {
        return this->_count_all_impl(query);
    }

    MySQLStmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement in " __FUNCTION__ "");
    this->bindConditionValues(stmtPtr, condition);
    this->executeQuery(stmtPtr);

    // 获取结果集
    MYSQL_STMT* stmt = stmtPtr.get();
    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    size_t count = 0;
    bind.buffer = &count;
    bind.buffer_length = sizeof(count);

    if (mysql_stmt_bind_result(stmt, &bind)) {
        throw DatabaseException("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }
    if (mysql_stmt_fetch(stmt) != 0) {
        throw DatabaseException("Failed to fetch result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    return count;
}

template<typename Entity, typename ID_TYPE>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::deleteByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        FK_SERVER_WARN("Condition is null in " __FUNCTION__ ", Cannot delete without condition");
        return { DbOperator::Status::InvalidParameters, 0 };
    }
    std::string query = FKUtils::concat(
        "DELETE FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause()
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    try {
        if (!isNeedBind) {
            return this->_delete_all_impl(query);
        }
        MySQLStmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement in " __FUNCTION__ "");
        this->bindConditionValues(stmtPtr, condition);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return { DbOperator::Status::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return { DbOperator::Status::Success, affectedRows };
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("" __FUNCTION__ " error: {}", e.what()));
        return { DbOperator::Status::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::deleteAll() const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [this](MYSQL* mysql) -> StatusWithAffectedRows {
            if (mysql_query(mysql, FKUtils::concat("TRUNCATE TABLE ", this->getTableName()).c_str())) {
                throw DatabaseException("Delete all failed: " + std::string(mysql_error(mysql))
                    + ". Table: " + this->getTableName());
            }
            const my_ulonglong affectedRows = mysql_affected_rows(mysql);
            if (affectedRows == 0) {
                return { DbOperator::Status::DataNotExist, 0 };
            }
            else if (affectedRows == static_cast<my_ulonglong>(-1)) {
                throw DatabaseException("Delete all failed: " + std::string(mysql_error(mysql)));
            }

            return { DbOperator::Status::Success, affectedRows };
        }
    );
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::updateFieldsByCondition(
    const std::unique_ptr<IQueryCondition>& condition,
    const std::vector<std::string>& fieldNames,
    Args&&... args) const
{
    static_assert((... && is_supported_param_v<Args>),
        "Unsupported parameter type. Use BindableParam or ExpressionParam");

    if (!condition || fieldNames.empty()) {
        FK_SERVER_WARN("Condition/Update fields is null in " __FUNCTION__ ", Cannot update without condition/fields");
        return { DbOperator::Status::InvalidParameters, 0 };
    }
    if (sizeof...(args) != fieldNames.size()) {
        throw DatabaseException("Number of field names must match number of values");
    }
    auto [setClauses, bindTuple] = this->buildSetClauseAndBindTuple(
        fieldNames,
        std::index_sequence_for<Args...>{},
        std::forward<Args>(args)...
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    try {
        std::string query = FKUtils::concat(
            "UPDATE ", this->getTableName(),
            " SET ", FKUtils::join_range(", ", setClauses),
            " WHERE ", condition->buildConditionClause());

        MySQLStmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr && !stmtPtr.isValid()) throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
        
        // 计算总参数数量：set需要绑定参数 + where条件参数
        const size_t setBindCount = std::tuple_size_v<decltype(bindTuple)>;
        const size_t totalParamCount = setBindCount + (isNeedBind ? condition->getParamCount() : 0);
        std::vector<MYSQL_BIND> binds(totalParamCount);
        std::memset(binds.data(), 0, sizeof(MYSQL_BIND) * totalParamCount);

        // 首先绑定set字段值参数，从0开始
        size_t startIndex = 0;
        std::apply([&](auto&&... bindArgs) {
            this->bindValues(stmtPtr, binds.data(), startIndex, std::forward<decltype(bindArgs)>(bindArgs)...);
            }, bindTuple);

        // 然后绑定where条件参数，从条件参数数量的位置开始
        if (isNeedBind) this->bindConditionValues(stmtPtr, condition, binds.data(), startIndex);

        // 一次性绑定所有参数
        MYSQL_STMT* stmt = stmtPtr;
        if (mysql_stmt_bind_param(stmt, binds.data())) {
            throw DatabaseException("Bind param failed: " + std::string(mysql_stmt_error(stmt)));
        }
        
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return { DbOperator::Status::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return { DbOperator::Status::Success, affectedRows };
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updateFieldsByCondition error: {}", e.what()));
        return { DbOperator::Status::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
MySQLStmtPtr FKBaseMapper<Entity, ID_TYPE>::prepareStatement(const std::string& query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [&](MYSQL* mysql) -> MySQLStmtPtr {
            MYSQL_STMT* stmt = mysql_stmt_init(mysql);
            if (!stmt) {
                throw std::runtime_error("Statement init failed");
            }

            MySQLStmtPtr stmtPtr(stmt);

            if (mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()))) {
                std::string error = mysql_error(mysql);
                throw std::runtime_error("Prepare failed: " + error);
            }

            return stmtPtr;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline void FKBaseMapper<Entity, ID_TYPE>::executeQuery(MySQLStmtPtr& stmtPtr) const
{
    MYSQL_STMT* stmt = stmtPtr;
    if (int code = mysql_stmt_execute(stmt)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Execute failed: " + error);
    }
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline void FKBaseMapper<Entity, ID_TYPE>::bindValues(MySQLStmtPtr& stmtPtr, Args&&... args) const
{
    MYSQL_STMT* stmt = stmtPtr;
    constexpr size_t count = sizeof...(Args);
    if (count == 0) return;

    std::vector<MYSQL_BIND> binds(count);
    std::memset(binds.data(), 0, sizeof(MYSQL_BIND) * count);  // 安全初始化

    // 使用折叠表达式绑定所有参数
    ValueBinder::bindParams(binds.data(), std::forward<Args>(args)...);

    if (mysql_stmt_bind_param(stmt, binds.data())) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
    //bindValues(stmtPtr, nullptr, 0, std::forward<Args>(args)...);
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline void FKBaseMapper<Entity, ID_TYPE>::bindValues(MySQLStmtPtr& stmtPtr, MYSQL_BIND* existingBinds, size_t& startIndex, Args&&... args) const
{
    MYSQL_STMT* stmt = stmtPtr;
    constexpr size_t count = sizeof...(Args);
    if (count == 0) return;

    std::vector<MYSQL_BIND> localBinds;
    MYSQL_BIND* bindsToUse = existingBinds;
    
    // 如果没有提供现有的binds，创建新的
    if (!existingBinds) {
        localBinds.resize(count);
        std::memset(localBinds.data(), 0, sizeof(MYSQL_BIND) * count);
        bindsToUse = localBinds.data();
    }

    // 使用折叠表达式绑定所有参数
    (ValueBinder::bindSingleValue(bindsToUse[startIndex++], std::forward<Args>(args)), ...);

    // 只有在使用本地binds时才需要调用mysql_stmt_bind_param
    if (!existingBinds) {
        if (mysql_stmt_bind_param(stmt, bindsToUse)) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind param failed: " + error);
        }
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ...Args, size_t ...Is>
inline auto FKBaseMapper<Entity, ID_TYPE>::buildSetClauseAndBindTuple(const std::vector<std::string>& fieldNames,
    std::index_sequence<Is...>, Args && ...args) const
{
    // 将参数包保存为 tuple，保留引用语义
    auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
    std::vector<std::string> setClauses;
    setClauses.reserve(sizeof...(Args));

    // 处理每个参数生成 SET 子句
    auto process_one = [&](size_t i, auto&& arg) {
        const std::string& fieldName = fieldNames[i];
        using ArgType = std::decay_t<decltype(arg)>;

        if constexpr (is_bindable_param<ArgType>::value) {
            setClauses.push_back(fieldName + "=?");
        }
        else if constexpr (is_expression_param<ArgType>::value) {
            setClauses.push_back(fieldName + "=" + arg.expr);
        }
        };

    // 按索引顺序处理每个参数
    (process_one(Is, std::get<Is>(argsTuple)), ...);

    // 构建绑定值 tuple：只包含 BindableParam 的值
    auto bindTuple = std::apply([&](auto&&... items) {
        return std::tuple_cat([&](auto&& item) -> auto {
            using ItemType = std::decay_t<decltype(item)>;
            if constexpr (is_bindable_param<ItemType>::value) {
                // 完美转发值（注意：确保存储的是值而非引用）
                return std::make_tuple(
                    std::forward<std::decay_t<decltype(item.value)>>(item.value)
                );
            }
            else {
                return std::tuple<>();
            }
            }(items) ...);
        }, argsTuple);

    return std::make_pair(setClauses, bindTuple);
}

template<typename Entity, typename ID_TYPE>
inline void FKBaseMapper<Entity, ID_TYPE>::bindConditionValues(MySQLStmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition) const
{
    size_t startIndex = 0;
    bindConditionValues(stmtPtr, condition, nullptr, startIndex);
}

template<typename Entity, typename ID_TYPE>
inline void FKBaseMapper<Entity, ID_TYPE>::bindConditionValues(MySQLStmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition, MYSQL_BIND* existingBinds, size_t& startIndex) const
{
    MYSQL_STMT* stmt = stmtPtr;
    size_t totalParams = condition->getParamCount();
    if (totalParams == 0) return;

    std::vector<MYSQL_BIND> localBinds;
    MYSQL_BIND* bindsToUse = existingBinds;
    if (!existingBinds) {
        localBinds.resize(totalParams);
        std::memset(localBinds.data(), 0, sizeof(MYSQL_BIND) * totalParams);
        bindsToUse = localBinds.data();
    }
    condition->bind(stmtPtr, bindsToUse, startIndex);

    if (!existingBinds) {
        if (mysql_stmt_bind_param(stmt, bindsToUse)) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind param failed: " + error);
        }
    }
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName>
inline constexpr auto FKBaseMapper<Entity, ID_TYPE>::buildOrderClause()
{
    constexpr auto direction = (Order == SortOrder::Ascending) ? " ASC" : " DESC";

    return FKUtils::concat(" ORDER BY ", FieldName, direction);
}

template<typename Entity, typename ID_TYPE>
template<Pagination Pagination>
inline constexpr auto FKBaseMapper<Entity, ID_TYPE>::buildLimitClause()
{
    if constexpr (Pagination.limit == 0 && Pagination.offset == 0) {
        return "";  // 无分页
    }
    else if constexpr (Pagination.limit == 0) {
        // 只有 OFFSET
        return FKUtils::concat(" LIMIT ", std::to_string(MAX_ROW_SIZE_U64), " OFFSET ", std::to_string(Pagination.offset));
    }
    else {
        // LIMIT 和 OFFSET
        return FKUtils::concat(" LIMIT ", std::to_string(Pagination.limit),
            " OFFSET ", std::to_string(Pagination.offset));
    }
}

template<typename Entity, typename ID_TYPE>
inline FKBaseMapper<Entity, ID_TYPE>::EntityRows FKBaseMapper<Entity, ID_TYPE>::_find_all_impl(const std::string& query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [this, &query](MYSQL* mysql) -> EntityRows {
            if (mysql_query(mysql, query.c_str())) {
                throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResultGuard resultGuard(mysql_store_result(mysql));
            if (!resultGuard.res) {
                throw DatabaseException("Store result failed");
            }

            EntityRows results;
            MYSQL_ROW row;
            unsigned long* lengths;
            MYSQL_FIELD* fields = mysql_fetch_fields(resultGuard.res);
            const int numFields = mysql_num_fields(resultGuard.res);

            while ((row = mysql_fetch_row(resultGuard.res))) {
                lengths = mysql_fetch_lengths(resultGuard.res);
                if (!lengths) {
                    throw DatabaseException("Lengths fetch error");
                }

                results.push_back(this->createEntityFromRow(row, fields, lengths, numFields));
            }

            return results;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline FKBaseMapper<Entity, ID_TYPE>::FieldMapRows FKBaseMapper<Entity, ID_TYPE>::_query_fields_impl(const std::string& query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [this, &query](MYSQL* mysql) -> FieldMapRows {
            if (mysql_query(mysql, query.c_str())) {
                throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResultGuard resultGuard(mysql_store_result(mysql));
            if (!resultGuard.res) {
                throw DatabaseException("Store result failed");
            }

            MYSQL_FIELD* fields = mysql_fetch_fields(resultGuard.res);
            const int numFields = mysql_num_fields(resultGuard.res);

            FieldMapRows results;
            MYSQL_ROW row;
            unsigned long* lengths;

            while ((row = mysql_fetch_row(resultGuard.res))) {
                lengths = mysql_fetch_lengths(resultGuard.res);
                if (!lengths) {
                    throw DatabaseException("Failed to get row lengths");
                }
                FieldMap fieldMap;
                for (int i = 0; i < numFields; ++i) {
                    const bool isNull = _parser.isFieldNull(row, i);
                    const std::string fieldName = fields[i].name;
                    auto fileType = fields[i].type;
                    switch (fileType) {
                        case MYSQL_TYPE_TINY:
                            fieldMap[fieldName] = fields[i].flags & UNSIGNED_FLAG
                                ? std::any(_parser.getValue<std::uint8_t>(row, i, lengths, isNull, fileType).value())
                                : std::any(_parser.getValue<std::int8_t>(row, i, lengths, isNull, fileType).value());
                        case MYSQL_TYPE_SHORT:
                            fieldMap[fieldName] = fields[i].flags & UNSIGNED_FLAG
                                ? std::any(_parser.getValue<std::uint16_t>(row, i, lengths, isNull, fileType).value())
                                : std::any(_parser.getValue<std::int16_t>(row, i, lengths, isNull, fileType).value());
                        case MYSQL_TYPE_INT24:
                        case MYSQL_TYPE_LONG:
                        case MYSQL_TYPE_YEAR:
                            fieldMap[fieldName] = fields[i].flags & UNSIGNED_FLAG
                                ? std::any(_parser.getValue<std::uint32_t>(row, i, lengths, isNull, fileType).value())
                                : std::any(_parser.getValue<std::int32_t>(row, i, lengths, isNull, fileType).value());
                            break;
                        case MYSQL_TYPE_LONGLONG:
                            fieldMap[fieldName] = fields[i].flags & UNSIGNED_FLAG
                                ? std::any(_parser.getValue<std::uint64_t>(row, i, lengths, isNull, fileType).value())
                                : std::any(_parser.getValue<std::int64_t>(row, i, lengths, isNull, fileType).value());
                            break;
                        case MYSQL_TYPE_FLOAT:
                            fieldMap[fieldName] = std::any(_parser.getValue<float>(row, i, lengths, isNull, fileType).value());
                            break;
                        case MYSQL_TYPE_DOUBLE:
                        case MYSQL_TYPE_DECIMAL:
                        case MYSQL_TYPE_NEWDECIMAL:
                            fieldMap[fieldName] = std::any(_parser.getValue<double>(row, i, lengths, isNull, fileType).value());
                            break;
                        case MYSQL_TYPE_TIMESTAMP:
                        case MYSQL_TYPE_DATETIME:
                            fieldMap[fieldName] = std::any(_parser.getValue
                                    <std::chrono::system_clock::time_point>(row, i, lengths, isNull, fileType).value());
                            break;
                        case MYSQL_TYPE_DATE:
                        case MYSQL_TYPE_TIME:
                        case MYSQL_TYPE_TINY_BLOB:
                        case MYSQL_TYPE_MEDIUM_BLOB:
                        case MYSQL_TYPE_LONG_BLOB:
                        case MYSQL_TYPE_BLOB:
                        case MYSQL_TYPE_VAR_STRING:
                        case MYSQL_TYPE_STRING:
                            fieldMap[fieldName] = std::any(_parser.getValue<std::string>(row, i, lengths, isNull, fileType).value());
                            break;
                        default:
                            fieldMap[fieldName] = std::any(_parser.getValue<std::string>(row, i, lengths, isNull, fileType).value());
                            break;
                    }
                }
                
                results.push_back(std::move(fieldMap));
            }

            return results;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline std::size_t FKBaseMapper<Entity, ID_TYPE>::_count_all_impl(const std::string& query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [this, &query](MYSQL* mysql) -> std::size_t {
            if (mysql_query(mysql, query.c_str())) {
                throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResultGuard resultGuard(mysql_store_result(mysql));
            if (!resultGuard.res) {
                throw DatabaseException("Store result failed");
            }
            //return resultGuard.res->row_count;

            MYSQL_ROW row = mysql_fetch_row(resultGuard.res);
            if (!row || !row[0]) {
                return 0;
            }

            return static_cast<size_t>(std::stoull(row[0]));
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline  FKBaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows FKBaseMapper<Entity, ID_TYPE>::_delete_all_impl(const std::string& query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [&query, this](MYSQL* mysql) -> StatusWithAffectedRows {
            if (mysql_query(mysql, query.c_str())) {
                throw DatabaseException("Delete all failed: " + std::string(mysql_error(mysql))
                    + ". Table: " + this->getTableName());
            }

            const my_ulonglong affectedRows = mysql_affected_rows(mysql);
            if (affectedRows == 0) {
                return { DbOperator::Status::DataNotExist, 0 };
            }
            else if (affectedRows == static_cast<my_ulonglong>(-1)) {
                throw DatabaseException("Delete all failed: " + std::string(mysql_error(mysql)));
            }

            return { DbOperator::Status::Success, affectedRows };
            }
        );
}
