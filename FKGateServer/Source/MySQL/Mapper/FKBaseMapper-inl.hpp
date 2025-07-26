#include "FKBaseMapper.hpp"
template<typename Entity, typename ID_TYPE>
inline const auto& FKBaseMapper<Entity, ID_TYPE>::getFieldMappings() const
{
    return _fieldMappings;
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
inline std::optional<ValueType> FKBaseMapper<Entity, ID_TYPE>::getFieldValue(const std::string& fieldName) const
{
    if (_fieldMappings.contains(fieldName)) {
        return std::get<ValueType>(_fieldMappings.at(fieldName));
    }
    return std::nullopt;
}

template<typename Entity, typename ID_TYPE>
inline MYSQL_TIME FKBaseMapper<Entity, ID_TYPE>::mysqlCurrentTime() const
{
    MYSQL_TIME value;
    memset(&value, 0, sizeof(value));
    auto now = std::chrono::system_clock::now();
    auto createTimeT = std::chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    // 获取 UTC 时间（TIMESTAMP 类型需要 UTC）
    struct tm timeinfo;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&timeinfo, &createTimeT);
#else
    localtime_r(&createTimeT, &timeinfo);
#endif
    // 填充 MYSQL_TIME 结构
    value.year = timeinfo.tm_year + 1900;
    value.month = timeinfo.tm_mon + 1;
    value.day = timeinfo.tm_mday;
    value.hour = timeinfo.tm_hour;
    value.minute = timeinfo.tm_min;
    value.second = timeinfo.tm_sec;
    value.second_part = millis * 1000;  // 毫秒转微秒
    value.time_type = MYSQL_TIMESTAMP_DATETIME;

    return value;
}

template<typename Entity, typename ID_TYPE>
inline std::optional<Entity> FKBaseMapper<Entity, ID_TYPE>::findById(ID_TYPE id)
{
    try {
        auto results = queryEntities<>(findByIdQuery(), id);

        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findById error: {}", e.what()));
        throw;
    }
}

template<typename Entity, typename ID_TYPE>
template <SortOrder Order, fixed_string FieldName, Pagination Pagination>
inline std::vector<Entity> FKBaseMapper<Entity, ID_TYPE>::findAll()
{
    try {
        std::string query = FKUtils::concat(findAllQuery(),
            _build_order_by_clause<Order, FieldName>(),
            _build_limit_clause<Pagination>());

        return FKMySQLConnectionPool::getInstance()->executeWithConnection(
            [this](MYSQL* mysql) -> std::vector<Entity> {
                if (mysql_query(mysql, query.c_str())) {
                    throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
                }
                ResultGuard resultGuard(mysql_store_result(mysql));
                if (!resultGuard.res) {
                    throw DatabaseException("Store result failed");
                }

                std::vector<Entity> results;
                int numFields = mysql_num_fields(resultGuard.res);
                MYSQL_ROW row;
                unsigned long* lengths;

                while ((row = mysql_fetch_row(resultGuard.res))) {
                    lengths = mysql_fetch_lengths(resultGuard.res);
                    if (!lengths) {
                        throw DatabaseException("Lengths fetch error");
                    }

                    Entity entity = createEntityFromRow(row, lengths);
                    results.push_back(std::move(entity));
                }

                return results;
            }
        );
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findAll error: {}", e.what()));
        throw;
    }
}

template<typename Entity, typename ID_TYPE>
inline DbOperator::Status FKBaseMapper<Entity, ID_TYPE>::insert(const Entity& entity)
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(insertQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for insert");
        }
        bindInsertParams(stmtPtr, entity);

        executeQuery(stmtPtr);
        return DbOperator::Status::Success;
    }
    catch (const std::exception& e) {
        std::string error = e.what();
        FK_SERVER_ERROR(std::format("insert error: {}", error));
        if (error.find("Duplicate entry") != std::string::npos) {
            return DbOperator::Status::DataAlreadyExist;
        }

        return DbOperator::Status::DatabaseError;
    }
}

template<typename Entity, typename ID_TYPE>
inline DbOperator::Status FKBaseMapper<Entity, ID_TYPE>::deleteById(ID_TYPE id)
{
    try {
        MySQLStmtPtr stmtPtr = prepareStatement(deleteByIdQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
        }

        bindValues(stmtPtr, id);
        executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return DbOperator::Status::Success;
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("deleteById error: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ...Args>
inline DbOperator::Status FKBaseMapper<Entity, ID_TYPE>::updateFieldsById(ID_TYPE id, const std::vector<std::string>& fieldNames, Args && ...args)
{
    try {
        // 参数数量检查
        if (sizeof...(args) != fieldNames.size()) {
            throw DatabaseException("Number of field names must match number of values");
        }

        // 构建安全的SET子句
        std::string setClause = fieldNames
            | std::views::transform([](const std::string& s) { return s + " = ?"; })
            | std::views::join_with(", ")
            | std::ranges::to<std::string>();

        MySQLStmtPtr stmtPtr = prepareStatement(FKUtils::concat("UPDATE ", getTableName(), " SET ", setClause,
            " WHERE id = ?"));
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
        }

        bindValues(stmtPtr, std::forward<Args>(args)..., id);
        executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }
        else if (affectedRows == static_cast<my_ulonglong>(-1)) {
            throw DatabaseException("Statement execution failed");
        }

        return DbOperator::Status::Success;
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updateFieldsByIdSafe error: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ConditionType, SortOrder Order, fixed_string FieldName,
    Pagination Pagination>
inline std::vector<std::unordered_map<std::string, std::string>> FKBaseMapper<Entity, ID_TYPE>::queryFieldsByCondition(
    const std::string& tableName,
    const std::vector<std::string>& fieldNames,
    const std::string& conditionField,
    const ConditionType& conditionValue) const
{
    // 构建sql并绑定执行
    MySQLStmtPtr stmtPtr = prepareStatement(FKUtils::concat(FKUtils::concat("SELECT ", FKUtils::plain_join_range(", ", fieldNames),
        " FROM ", tableName, " WHERE ", conditionField, " = ?"),
        _build_order_by_clause<Order, FieldName>(),
        _build_limit_clause<Pagination>()));

    if (!stmtPtr) throw DatabaseException("Failed to prepare statement in " __FUNCTION__ "");
    bindValues(stmtPtr, conditionValue);
    executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    StmtOutputRes output;
    _process_stmt_result(stmt, output);
    if (output.rowCount == 0)  return {};

    // 获取所有行的结果
    std::vector<std::unordered_map<std::string, std::string>> results;
    while (true) {
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result == MYSQL_NO_DATA) {
            break; // 没有更多数据
        }
        else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {// 数据截断
            throw DatabaseException("Fetch failed in fetchFieldsByCondition with error code: " + std::to_string(fetch_result));
        }

        // 处理当前行的数据
        std::unordered_map<std::string, std::string> rowData;
        for (size_t i = 0; i < fieldNames.size(); ++i) {
            if (output.isNull[i]) {
                rowData[fieldNames[i]] = {};
            }
            else {
                rowData[fieldNames[i]] = std::string(output.buffers[i].data(), output.lengths[i]);
            }
        }
        results.push_back(std::move(rowData));
    }

    return results;
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName, Pagination Pagination,
    typename... Args >
inline std::vector<Entity> FKBaseMapper<Entity, ID_TYPE>::queryEntities(std::string&& sql, Args && ...args) const
{
    MySQLStmtPtr stmtPtr = prepareStatement(FKUtils::concat(std::move(sql),
        _build_order_by_clause<Order, FieldName>(),
        _build_limit_clause<Pagination>()));
    if (!stmtPtr.isValid()) {
        throw DatabaseException("Failed to prepare statement for " __FUNCTION__ "");
    }
    bindValues(stmtPtr, std::forward<Args>(args)...);
    executeQuery(stmtPtr);

    MYSQL_STMT* stmt = stmtPtr.get();
    StmtOutputRes output;
    _process_stmt_result(stmt, output);
    if (output.rowCount == 0)  return {};

    std::vector<const char*> rowData(output.columnCount);
    std::vector<Entity> results;
    results.reserve(output.rowCount);

    while (true) {
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result == MYSQL_NO_DATA) {
            break;
        }
        else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
            throw DatabaseException("Fetch failed in queryEntities with error code: " + std::to_string(fetch_result));
        }

        for (size_t i = 0; i < output.columnCount; i++) {
            if (output.isNull[i]) {
                rowData[i] = {};
            }
            else {
                rowData[i] = static_cast<char*>(output.buffers[i].data());
            }
        }

        results.push_back(createEntityFromRow(const_cast<char**>(rowData.data()), output.lengths.get()));
    }

    return results;
}

template<typename Entity, typename ID_TYPE>
MySQLStmtPtr FKBaseMapper<Entity, ID_TYPE>::prepareStatement(std::string query) const
{
    return FKMySQLConnectionPool::getInstance()->executeWithConnection(
        [query](MYSQL* mysql) -> MySQLStmtPtr {
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
    if (mysql_stmt_execute(stmt)) {
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
    bindParams(binds.data(), std::forward<Args>(args)...);

    if (mysql_stmt_bind_param(stmt, binds.data())) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
void FKBaseMapper<Entity, ID_TYPE>::bindSingleValue(MYSQL_BIND& bind, ValueType&& value) const
{
    using RawType = std::remove_cv_t<std::remove_reference_t<ValueType>>;
    //using Type = std::decay_t<ValueType>; // 类型退化、数组→指针转换、函数→函数指针
    if constexpr (std::is_integral_v<RawType>) {
        if constexpr (std::is_same_v<RawType, bool>) {
            bind.buffer_type = MYSQL_TYPE_TINY;
        }
        else if constexpr (sizeof(RawType) <= 1) {
            bind.buffer_type = MYSQL_TYPE_TINY;
        }
        else if constexpr (sizeof(RawType) <= 2) {
            bind.buffer_type = MYSQL_TYPE_SHORT;
        }
        else if constexpr (sizeof(RawType) <= 4) {
            bind.buffer_type = MYSQL_TYPE_LONG;
        }
        else {
            bind.buffer_type = MYSQL_TYPE_LONGLONG;
        }

        bind.is_unsigned = std::is_unsigned_v<RawType>;
        bind.buffer = const_cast<RawType*>(&value);
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (std::is_floating_point_v<RawType>) {
        if constexpr (std::is_same_v<RawType, float>) {
            bind.buffer_type = MYSQL_TYPE_FLOAT;
        }
        else {
            bind.buffer_type = MYSQL_TYPE_DOUBLE;
        }

        bind.buffer = const_cast<RawType*>(&value);
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (is_varchar_type_v<RawType>) {
        bind.buffer_type = MYSQL_TYPE_VAR_STRING;
        bind.buffer = const_cast<char*>(value.val);
        bind.buffer_length = value.len + 1;
        bind.length = &value.len;
        bind.is_null = 0;
    }
    else if constexpr (std::is_same_v<RawType, MYSQL_TIME>) {
        bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
        bind.buffer = const_cast<MYSQL_TIME*>(&value);
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (is_optional_type_v<RawType>) {
        if (value.has_value()) {
            bindSingleValue(bind, *value);
        }
        else {
            bind.buffer_type = MYSQL_TYPE_NULL;
            bind.is_null = 1;
        }
    }
    else {
        throw DatabaseException("Unsupported value type in bindSingleValue");
    }
}

template<typename Entity, typename ID_TYPE>
template<SortOrder Order, fixed_string FieldName>
inline constexpr auto FKBaseMapper<Entity, ID_TYPE>::_build_order_by_clause()
{
    constexpr auto direction = (Order == SortOrder::Ascending) ? " ASC" : " DESC";

    return FKUtils::concat(" ORDER BY ", FieldName, direction);
}

template<typename Entity, typename ID_TYPE>
template<Pagination pagination>
inline constexpr auto FKBaseMapper<Entity, ID_TYPE>::_build_limit_clause()
{
    if constexpr (pagination.limit == 0 && pagination.offset == 0) {
        return "";  // 无分页
    }
    else if constexpr (pagination.limit == 0) {
        // 只有 OFFSET
        return FKUtils::concat(" LIMIT ", std::to_string(MAX_ROW_SIZE), " OFFSET ", std::to_string(pagination.offset));
    }
    else {
        // LIMIT 和 OFFSET
        return FKUtils::concat(" LIMIT ", std::to_string(pagination.limit),
            " OFFSET ", std::to_string(pagination.offset));
    }
}

template<typename Entity, typename ID_TYPE>
inline void FKBaseMapper<Entity, ID_TYPE>::_process_stmt_result(MYSQL_STMT* stmt, StmtOutputRes& output) const
{
    if (mysql_stmt_store_result(stmt)) {
        throw DatabaseException("Failed to store result in " __FUNCTION__ ": " +
            std::string(mysql_stmt_error(stmt)));
    }
    // 检查结果集是否为空
    if (!(output.rowCount = mysql_stmt_num_rows(stmt))) return;

    // 获取结果集元数据
    ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
    if (!resultGuard.res)  throw DatabaseException("No metadata available for fetchFieldsByCondition query");

    // 获取字段数量
    output.columnCount = mysql_num_fields(resultGuard.res); // fieldNames.size()
    output.binds = std::make_unique<MYSQL_BIND[]>(output.columnCount);
    output.isNull = std::make_unique<char[]>(output.columnCount);
    output.lengths = std::make_unique<unsigned long[]>(output.columnCount);
    output.error = std::make_unique<char[]>(output.columnCount);
    std::fill_n(output.isNull.get(), output.columnCount, 0);
    std::fill_n(output.error.get(), output.columnCount, 0);

    // 绑定结果集
    output.buffers = std::vector<std::vector<char>>(output.columnCount);
    for (int i = 0; i < output.columnCount; i++) {
        MYSQL_FIELD* field = mysql_fetch_field_direct(resultGuard.res, i);

        unsigned long bufferSize = (field->type == MYSQL_TYPE_VAR_STRING || field->type == MYSQL_TYPE_STRING)
            ? field->length >> 2 : field->length;
        output.buffers[i].resize(bufferSize + 1);

        memset(&output.binds[i], 0, sizeof(MYSQL_BIND));
        output.binds[i].buffer_type = mysql_fetch_field_direct(resultGuard.res, i)->type;
        output.binds[i].buffer = output.buffers[i].data();
        output.binds[i].buffer_length = bufferSize;
        output.binds[i].is_null = reinterpret_cast<bool*>(&output.isNull[i]);
        output.binds[i].length = &output.lengths[i];
        output.binds[i].error = reinterpret_cast<bool*>(&output.error[i]);
    }
    if (mysql_stmt_bind_result(stmt, output.binds.get())) {
        throw DatabaseException("Failed to bind result in " __FUNCTION__ ": " +
            std::string(mysql_stmt_error(stmt)));
    }
}
