#ifndef UNIVERSAL_MYSQL_HEADER_ONLY
#endif
#include "base_mapper.hpp"

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
        LOGGER_WARN("IN / NOT IN / AND / OR condition must be set values " __FUNCTION__ "");\
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
namespace universal::mysql {

template<typename Entity, typename ID_TYPE>
inline const auto& BaseMapper<Entity, ID_TYPE>::getFieldMappings() const
{
    return _fieldMappings;
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline std::optional<ValueType> BaseMapper<Entity, ID_TYPE>::getFieldValue(const std::string& fieldName) const
{
    if (auto it = _fieldMappings.find(fieldName); it != _fieldMappings.end()) {
        return std::get<ValueType>(it->second);
    }
    return std::nullopt;
}

template<typename Entity, typename ID_TYPE>
inline std::optional<Entity> BaseMapper<Entity, ID_TYPE>::findById(ID_TYPE id)
{
    auto results = this->queryEntities<>(findByIdQuery(), id);

    if (results.empty()) {
        return std::nullopt;
    }

    return results.front();
}

template<typename Entity, typename ID_TYPE>
template <SortOrder Order, utils::string::fixed_string FieldName, Pagination Pagination>
inline BaseMapper<Entity, ID_TYPE>::EntityRows BaseMapper<Entity, ID_TYPE>::findAll()
{
    std::string query = utils::string::concat(findAllQuery(),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>());
    this->_find_all_impl(query);
}

template<typename Entity, typename ID_TYPE>
inline BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::insert(const Entity& entity)
{
    try {
        std::string query = insertQuery();
        StmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr) {
            throw std::runtime_error("Failed to prepare statement for insert");
        }
        this->bindInsertParams(stmtPtr, entity);
        this->executeQuery(stmtPtr);
        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr.get());

        if (affectedRows == 0) {
            return { DbOperatorStatus::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw std::runtime_error("Statement execution failed");
        }

        return { DbOperatorStatus::Success, affectedRows };
    }
    catch (const std::exception& e) {
        std::string error = e.what();
        LOGGER_ERROR(std::format("insert error: {}", error));
        if (error.find("Duplicate entry") != std::string::npos) {
            return { DbOperatorStatus::DataAlreadyExist, 0 };
        }
        return { DbOperatorStatus::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
inline  BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::deleteById(ID_TYPE id)
{
    try {
        std::string query = deleteByIdQuery();
        StmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr) throw std::runtime_error("Failed to prepare statement for " __FUNCTION__ "");

        this->bindValues(stmtPtr, id);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return { DbOperatorStatus::DataNotExist, 0};
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw std::runtime_error("Statement execution failed");
        }

        return { DbOperatorStatus::Success, affectedRows };
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("deleteById error: {}", e.what()));
        return { DbOperatorStatus::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ...Args>
inline  BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::updateFieldsById(
     ID_TYPE id,
    const std::vector<std::string>& fieldNames, Args && ...args)
{
    if (fieldNames.empty()) {
        LOGGER_ERROR("Update fields is null in " __FUNCTION__ ", Cannot update without fields");
        return { DbOperatorStatus::InvalidParameters, 0 };
    }
    try {
        // 参数数量检查
        if (sizeof...(args) != fieldNames.size()) {
            throw std::runtime_error("Number of field names must match number of values");
        }

        std::string query = fieldNames.size() > 1
            ? utils::string::concat(
                "UPDATE ", this->getTableName(),
                " SET ", utils::string::join_range(" = ?, ", fieldNames),
                " = ? WHERE  id = ?")
            : utils::string::concat(
                "UPDATE ", this->getTableName(),
                " SET ", fieldNames[0],
                " = ? WHERE  id = ?");
        StmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr) throw std::runtime_error("Failed to prepare statement for " __FUNCTION__ "");

        this->bindValues(stmtPtr, std::forward<Args>(args)..., id);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return { DbOperatorStatus::DataNotExist, 0 };

        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw std::runtime_error("Statement execution failed");
        }
        return { DbOperatorStatus::Success, affectedRows };
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("updateFieldsByIdSafe error: {}", e.what()));
        return { DbOperatorStatus::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, utils::string::fixed_string FieldName, Pagination Pagination,
    typename... Args >
inline BaseMapper<Entity, ID_TYPE>::EntityRows BaseMapper<Entity, ID_TYPE>::queryEntities(std::string&& sql, Args && ...args) const
{
    // 构建sql初始化
    std::string query = utils::string::concat(std::move(sql),
        this->buildOrderClause<Order, FieldName>(),
        this->buildLimitClause<Pagination>());
    StmtPtr stmtPtr = this->prepareStatement(query);

    if (!stmtPtr)  throw std::runtime_error("Failed to prepare statement for " __FUNCTION__ "");
    // 绑定条件参数并执行查询
    this->bindValues(stmtPtr, std::forward<Args>(args)...);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResPtr resPtr(mysql_stmt_result_metadata(stmt));
    if (!resPtr)  throw std::runtime_error("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resPtr.get());
    MYSQL_FIELD* fields = mysql_fetch_fields(resPtr.get());

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw std::runtime_error("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw std::runtime_error("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
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
            throw std::runtime_error("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
        }
        results.push_back(createEntityFromBinds(binds.get(), fields, lengths.get(),
            isNull.get(), columnCount));
    }
    return results;
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, utils::string::fixed_string FieldName, Pagination Pagination>
inline BaseMapper<Entity, ID_TYPE>::EntityRows BaseMapper<Entity, ID_TYPE>::queryEntitiesByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        LOGGER_WARN("Condition is null in " __FUNCTION__ "");
        return {};
    }
    std::string query = utils::string::concat(
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
    
    StmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr)  throw std::runtime_error("Failed to prepare statement for " __FUNCTION__ "");
    this->bindConditionValues(stmtPtr, condition);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResPtr resPtr(mysql_stmt_result_metadata(stmt));
    if (!resPtr)  throw std::runtime_error("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resPtr.get());
    MYSQL_FIELD* fields = mysql_fetch_fields(resPtr.get());

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw std::runtime_error("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw std::runtime_error("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
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
            throw std::runtime_error("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
        }
        results.push_back(createEntityFromBinds(binds.get(), fields, lengths.get(),
            isNull.get(), columnCount));
    }
    return results;
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, utils::string::fixed_string FieldName, Pagination Pagination>
inline BaseMapper<Entity, ID_TYPE>::FieldMapRows BaseMapper<Entity, ID_TYPE>::queryFieldsByCondition(
    const std::unique_ptr<IQueryCondition>& condition, 
    const std::vector<std::string>& fieldNames) const
{
    if (!condition) {
        LOGGER_WARN("Condition is null in " __FUNCTION__ "");
        return {};
    }
    
    if (fieldNames.empty()) {
        LOGGER_WARN("Field names is empty in " __FUNCTION__ "");
        return {};
    }
    
    // 构建查询语句
    std::string query = utils::string::concat(
        "SELECT ", utils::string::join_range(", ", fieldNames),
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

    StmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr) throw std::runtime_error("Failed to prepare statement in " __FUNCTION__ "");
    this->bindConditionValues(stmtPtr, condition);
    this->executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    ResPtr resPtr(mysql_stmt_result_metadata(stmt));
    if (!resPtr)  throw std::runtime_error("No metadata available for fetchFieldsByCondition query");
    std::size_t columnCount = mysql_num_fields(resPtr.get());
    MYSQL_FIELD* fields = mysql_fetch_fields(resPtr.get());

    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    std::memset(binds.get(), 0, columnCount * sizeof(MYSQL_BIND));
    std::memset(lengths.get(), 0, columnCount * sizeof(unsigned long));
    std::memset(isNull.get(), 0, columnCount * sizeof(char));
    std::vector<std::vector<char>> buffers(columnCount);
    MYSQL_FOREACH_BIND_RESSET(fields, binds, lengths, isNull, buffers, columnCount)

    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw std::runtime_error("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    if (mysql_stmt_store_result(stmt)) {
        throw std::runtime_error("Failed to store result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
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
            throw std::runtime_error("Fetch failed: " + std::string(mysql_stmt_error(stmt)));
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
inline auto BaseMapper<Entity, ID_TYPE>::getFieldMapValue(const FieldMap& fieldMap, const std::string& fieldName) const
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
        LOGGER_ERROR("Type mismatch for key: " + fieldName);
        return { std::nullopt, GetFieldMapResStatus::BadCast };
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline ValueType BaseMapper<Entity, ID_TYPE>::getFieldMapValueOrDefault(const FieldMap& fieldMap,
    const std::string& fieldName, ValueType defaultValue) const
{
    auto [value, status] = this->getFieldMapValue<ValueType>(fieldMap, fieldName);
    return value.value_or(std::move(defaultValue));
}

template<typename Entity, typename ID_TYPE>
inline size_t BaseMapper<Entity, ID_TYPE>::countByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        LOGGER_WARN("Condition is null in " __FUNCTION__ "");
        return 0;
    }
    std::string query = utils::string::concat(
        "SELECT COUNT(*) FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause()
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    if (!isNeedBind) {
        return this->_count_all_impl(query);
    }

    StmtPtr stmtPtr = this->prepareStatement(query);
    if (!stmtPtr) throw std::runtime_error("Failed to prepare statement in " __FUNCTION__ "");
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
        throw std::runtime_error("Failed to bind result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }
    if (mysql_stmt_fetch(stmt) != 0) {
        throw std::runtime_error("Failed to fetch result in " __FUNCTION__ ": " + std::string(mysql_stmt_error(stmt)));
    }

    return count;
}

template<typename Entity, typename ID_TYPE>
inline BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::deleteByCondition(
    const std::unique_ptr<IQueryCondition>& condition) const
{
    if (!condition) {
        LOGGER_WARN("Condition is null in " __FUNCTION__ ", Cannot delete without condition");
        return { DbOperatorStatus::InvalidParameters, 0 };
    }
    std::string query = utils::string::concat(
        "DELETE FROM ", this->getTableName(),
        " WHERE ", condition->buildConditionClause()
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    try {
        if (!isNeedBind) {
            return this->_delete_all_impl(query);
        }
        StmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr) throw std::runtime_error("Failed to prepare statement in " __FUNCTION__ "");
        this->bindConditionValues(stmtPtr, condition);
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return { DbOperatorStatus::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw std::runtime_error("Statement execution failed");
        }

        return { DbOperatorStatus::Success, affectedRows };
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("" __FUNCTION__ " error: {}", e.what()));
        return { DbOperatorStatus::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
inline  BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::deleteAll(bool confirm) const
{
    if (!confirm) {
        LOGGER_WARN("WARNING: This will permanently delete ALL DATA! You should confirm (true) the operation.");
        return { DbOperatorStatus::InvalidParameters, 0 };
    }
    return _pool->execute_with_connection(
        [this](MYSQL* mysql) -> StatusWithAffectedRows {
            if (mysql_query(mysql, utils::string::concat("TRUNCATE TABLE ", this->getTableName()).c_str())) {
                throw std::runtime_error("Delete all failed: " + std::string(mysql_error(mysql))
                    + ". Table: " + this->getTableName());
            }
            const my_ulonglong affectedRows = mysql_affected_rows(mysql);
            if (affectedRows == 0) {
                return { DbOperatorStatus::DataNotExist, 0 };
            }
            else if (affectedRows == static_cast<my_ulonglong>(-1)) {
                throw std::runtime_error("Delete all failed: " + std::string(mysql_error(mysql)));
            }

            return { DbOperatorStatus::Success, affectedRows };
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline bool BaseMapper<Entity, ID_TYPE>::createTable() 
{
    return _pool->execute_with_connection(
        [this](MYSQL* mysql) -> std::size_t {
            if (mysql_query(mysql, createTableQuery().c_str())) {
                throw std::runtime_error("Query failed: " + std::string(mysql_error(mysql)));
                return false;
            }
            return true;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline bool BaseMapper<Entity, ID_TYPE>::dropTable(bool confirm)
{
    if (!confirm) {
        LOGGER_WARN("WARNING: This will permanently delete TABLE and ALL DATA! You should confirm (true) the operation.");
        return false;
    }
    return _pool->execute_with_connection(
        [this](MYSQL* mysql) -> std::size_t {
            if (mysql_query(mysql, utils::string::concat("DROP TABLE IF EXISTS ", this->getTableName()).c_str())) {
                throw std::runtime_error("Query failed: " + std::string(mysql_error(mysql)));
                return false;
            }
            return true;
        }
    );
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline  BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::updateFieldsByCondition(
    const std::unique_ptr<IQueryCondition>& condition,
    const std::vector<std::string>& fieldNames,
    Args&&... args) const
{
    static_assert((... && is_supported_param_v<Args>),
        "Unsupported parameter type. Use BindableParam or ExpressionParam");

    if (!condition || fieldNames.empty()) {
        LOGGER_WARN("Condition/Update fields is null in " __FUNCTION__ ", Cannot update without condition/fields");
        return { DbOperatorStatus::InvalidParameters, 0 };
    }
    if (sizeof...(args) != fieldNames.size()) {
        throw std::runtime_error("Number of field names must match number of values");
    }
    auto [setClauses, bindTuple] = this->buildSetClauseAndBindTuple(
        fieldNames,
        std::index_sequence_for<Args...>{},
        std::forward<Args>(args)...
    );
    bool isNeedBind = true;
    MSYSQL_QUERY_CONDITION_JUDGMENT(isNeedBind);
    try {
        std::string query = utils::string::concat(
            "UPDATE ", this->getTableName(),
            " SET ", utils::string::join_range(", ", setClauses),
            " WHERE ", condition->buildConditionClause());

        StmtPtr stmtPtr = this->prepareStatement(query);
        if (!stmtPtr) throw std::runtime_error("Failed to prepare statement for " __FUNCTION__ "");
        
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
            throw std::runtime_error("Bind param failed: " + std::string(mysql_stmt_error(stmt)));
        }
        
        this->executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return { DbOperatorStatus::DataNotExist, 0 };
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw std::runtime_error("Statement execution failed");
        }

        return { DbOperatorStatus::Success, affectedRows };
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("updateFieldsByCondition error: {}", e.what()));
        return { DbOperatorStatus::DatabaseError, 0 };
    }
}

template<typename Entity, typename ID_TYPE>
StmtPtr BaseMapper<Entity, ID_TYPE>::prepareStatement(const std::string& query) const
{
    return _pool->execute_with_connection(
        [&](MYSQL* mysql) -> StmtPtr {
            MYSQL_STMT* stmt = mysql_stmt_init(mysql);
            if (!stmt) {
                throw std::runtime_error("Statement init failed");
            }

            StmtPtr stmtPtr(stmt);

            if (mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()))) {
                std::string error = mysql_error(mysql);
                throw std::runtime_error("Prepare failed: " + error);
            }

            return stmtPtr;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline void BaseMapper<Entity, ID_TYPE>::executeQuery(StmtPtr& stmtPtr) const
{
    MYSQL_STMT* stmt = stmtPtr.get();
    if (int code = mysql_stmt_execute(stmt)) {
        std::string error = mysql_stmt_error(stmt);
        throw std::runtime_error("Execute failed: " + error);
    }
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline void BaseMapper<Entity, ID_TYPE>::bindValues(StmtPtr& stmtPtr, Args&&... args) const
{
    MYSQL_STMT* stmt = stmtPtr.get();
    constexpr size_t count = sizeof...(Args);
    if (count == 0) return;

    std::vector<MYSQL_BIND> binds(count);
    std::memset(binds.data(), 0, sizeof(MYSQL_BIND) * count);  // 安全初始化

    // 使用折叠表达式绑定所有参数
    bindParams(binds.data(), std::forward<Args>(args)...);

    if (mysql_stmt_bind_param(stmt, binds.data())) {
        std::string error = mysql_stmt_error(stmt);
        throw std::runtime_error("Bind param failed: " + error);
    }
    //bindValues(stmtPtr, nullptr, 0, std::forward<Args>(args)...);
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline void BaseMapper<Entity, ID_TYPE>::bindValues(StmtPtr& stmtPtr, MYSQL_BIND* existingBinds, size_t& startIndex, Args&&... args) const
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
    (bindSingleValue(bindsToUse[startIndex++], std::forward<Args>(args)), ...);

    // 只有在使用本地binds时才需要调用mysql_stmt_bind_param
    if (!existingBinds) {
        if (mysql_stmt_bind_param(stmt, bindsToUse)) {
            std::string error = mysql_stmt_error(stmt);
            throw std::runtime_error("Bind param failed: " + error);
        }
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ...Args, size_t ...Is>
inline auto BaseMapper<Entity, ID_TYPE>::buildSetClauseAndBindTuple(const std::vector<std::string>& fieldNames,
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
inline void BaseMapper<Entity, ID_TYPE>::bindConditionValues(StmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition) const
{
    size_t startIndex = 0;
    bindConditionValues(stmtPtr, condition, nullptr, startIndex);
}

template<typename Entity, typename ID_TYPE>
inline void BaseMapper<Entity, ID_TYPE>::bindConditionValues(StmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition, MYSQL_BIND* existingBinds, size_t& startIndex) const
{
    MYSQL_STMT* stmt = stmtPtr.get();
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
            throw std::runtime_error("Bind param failed: " + error);
        }
    }
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, utils::string::fixed_string FieldName>
inline constexpr auto BaseMapper<Entity, ID_TYPE>::buildOrderClause()
{
    constexpr auto direction = (Order == SortOrder::Ascending) ? " ASC" : " DESC";

    return utils::string::concat(" ORDER BY ", FieldName, direction);
}

template<typename Entity, typename ID_TYPE>
template<Pagination Pagination>
inline constexpr auto BaseMapper<Entity, ID_TYPE>::buildLimitClause()
{
    if constexpr (Pagination.limit == 0 && Pagination.offset == 0) {
        return "";  // 无分页
    }
    else if constexpr (Pagination.limit == 0) {
        // 只有 OFFSET
        return utils::string::concat(" LIMIT ", std::to_string(MAX_ROW_SIZE_U64), " OFFSET ", std::to_string(Pagination.offset));
    }
    else {
        // LIMIT 和 OFFSET
        return utils::string::concat(" LIMIT ", std::to_string(Pagination.limit),
            " OFFSET ", std::to_string(Pagination.offset));
    }
}

template<typename Entity, typename ID_TYPE>
inline BaseMapper<Entity, ID_TYPE>::EntityRows BaseMapper<Entity, ID_TYPE>::_find_all_impl(const std::string& query) const
{
    return _pool->execute_with_connection(
        [this, &query](MYSQL* mysql) -> EntityRows {
            if (mysql_query(mysql, query.c_str())) {
                throw std::runtime_error("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResPtr resPtr(mysql_store_result(mysql));
            if (!resPtr) {
                throw std::runtime_error("Store result failed");
            }

            EntityRows results;
            MYSQL_ROW row;
            unsigned long* lengths;
            MYSQL_FIELD* fields = mysql_fetch_fields(resPtr.get());
            const int numFields = mysql_num_fields(resPtr.get());

            while ((row = mysql_fetch_row(resPtr.get()))) {
                lengths = mysql_fetch_lengths(resPtr.get());
                if (!lengths) {
                    throw std::runtime_error("Lengths fetch error");
                }

                results.push_back(this->createEntityFromRow(row, fields, lengths, numFields));
            }

            return results;
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline BaseMapper<Entity, ID_TYPE>::FieldMapRows BaseMapper<Entity, ID_TYPE>::_query_fields_impl(const std::string& query) const
{
    return _pool->execute_with_connection(
        [this, &query](MYSQL* mysql) -> FieldMapRows {
            if (mysql_query(mysql, query.c_str())) {
                throw std::runtime_error("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResPtr resPtr(mysql_store_result(mysql));
            if (!resPtr) {
                throw std::runtime_error("Store result failed");
            }

            MYSQL_FIELD* fields = mysql_fetch_fields(resPtr.get());
            const int numFields = mysql_num_fields(resPtr.get());

            FieldMapRows results;
            MYSQL_ROW row;
            unsigned long* lengths;

            while ((row = mysql_fetch_row(resPtr.get()))) {
                lengths = mysql_fetch_lengths(resPtr.get());
                if (!lengths) {
                    throw std::runtime_error("Failed to get row lengths");
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
inline std::size_t BaseMapper<Entity, ID_TYPE>::_count_all_impl(const std::string& query) const
{
    return _pool->execute_with_connection(
        [this, &query](MYSQL* mysql) -> std::size_t {
            if (mysql_query(mysql, query.c_str())) {
                throw std::runtime_error("Query failed: " + std::string(mysql_error(mysql)));
            }
            ResPtr resPtr(mysql_store_result(mysql));
            if (!resPtr) {
                throw std::runtime_error("Store result failed");
            }
            MYSQL_ROW row = mysql_fetch_row(resPtr.get());
            if (!row || !row[0]) {
                return 0;
            }

            return static_cast<size_t>(std::stoull(row[0]));
        }
    );
}

template<typename Entity, typename ID_TYPE>
inline  BaseMapper<Entity, ID_TYPE>::StatusWithAffectedRows BaseMapper<Entity, ID_TYPE>::_delete_all_impl(const std::string& query) const
{
    return _pool->execute_with_connection(
        [&query, this](MYSQL* mysql) -> StatusWithAffectedRows {
            if (mysql_query(mysql, query.c_str())) {
                throw std::runtime_error("Delete all failed: " + std::string(mysql_error(mysql))
                    + ". Table: " + this->getTableName());
            }

            const my_ulonglong affectedRows = mysql_affected_rows(mysql);
            if (affectedRows == 0) {
                return { DbOperatorStatus::DataNotExist, 0 };
            }
            else if (affectedRows == static_cast<my_ulonglong>(-1)) {
                throw std::runtime_error("Delete all failed: " + std::string(mysql_error(mysql)));
            }

            return { DbOperatorStatus::Success, affectedRows };
            }
        );
}

}// namespace universal::mysql