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
        MySQLStmtPtr stmtPtr = prepareStatement(findByIdQuery());
        if (!stmtPtr.isValid()) {
            throw DatabaseException("Failed to prepare statement for findById");
        }

        bindValues(stmtPtr, id);
        executeQuery(stmtPtr);

        return fetchSingleResult(stmtPtr);
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findById error: {}", e.what()));
        throw;
    }
}

template<typename Entity, typename ID_TYPE>
template <OrderBy orderBy, Pagination pagination>
inline std::vector<Entity> FKBaseMapper<Entity, ID_TYPE>::findAll()
{
    try {
        std::string query = FKUtils::concat(findAllQuery(),
            buildOrderByClause<orderBy>(),
            buildLimitClause<pagination>());

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
            throw DatabaseException("Failed to prepare statement for deleteById");
        }

        bindValues(stmtPtr, id);
        executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);

        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
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
            throw DatabaseException("Failed to prepare statement for updateFieldsByIdSafe");
        }

        bindValues(stmtPtr, std::forward<Args>(args)..., id);
        executeQuery(stmtPtr);

        my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
        if (affectedRows == 0) {
            return DbOperator::Status::DataNotExist;
        }

        return DbOperator::Status::Success;
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("updateFieldsByIdSafe error: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ConditionType>
inline std::vector<std::unordered_map<std::string, std::string>> FKBaseMapper<Entity, ID_TYPE>::fetchFieldsByCondition(
    const std::string& tableName,
    const std::vector<std::string>& fieldNames,
    const std::string& conditionField,
    const ConditionType& conditionValue) const
{
    // 构建字段列表
    std::string fieldList = fieldNames
        | std::views::transform([](const std::string& s) { return s + " = ?"; })
        | std::views::join_with(", ")
        | std::ranges::to<std::string>();

    // 构建sql并绑定执行
    MySQLStmtPtr stmtPtr = prepareStatement(FKUtils::concat("SELECT ", fieldList,
        " FROM ", tableName, " WHERE ", conditionField, " = ?"));
    if (!stmtPtr) throw DatabaseException("Failed to prepare statement in fetchFieldsByCondition");
    MYSQL_STMT* stmt = stmtPtr.get();

    bindValues(stmtPtr, conditionValue);
    executeQuery(stmtPtr);

    // 检查结果集是否为空
    my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
    if (rowCount == 0) return {};

    // 获取结果集元数据
    ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
    if (!resultGuard.res)  throw DatabaseException("No metadata available for fetchFieldsByCondition query");

    // 获取字段数量
    std::size_t columnCount = mysql_num_fields(resultGuard.res); // fieldNames.size()
    auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
    auto isNull = std::make_unique<char[]>(columnCount);
    auto lengths = std::make_unique<unsigned long[]>(columnCount);
    auto error = std::make_unique<char[]>(columnCount);
    std::fill_n(isNull.get(), columnCount, 0);
    std::fill_n(error.get(), columnCount, 0);

    // 绑定结果集
    std::vector<std::vector<char>> buffers(columnCount);
    for (int i = 0; i < columnCount; i++) {
        MYSQL_FIELD* field = mysql_fetch_field_direct(resultGuard.res, i);

        unsigned long bufferSize = (field->type == MYSQL_TYPE_VAR_STRING || field->type == MYSQL_TYPE_STRING)
            ? field->length >> 2 : field->length;
        buffers[i].resize(bufferSize + 1);

        memset(&binds[i], 0, sizeof(MYSQL_BIND));
        binds[i].buffer_type = mysql_fetch_field_direct(resultGuard.res, i)->type;
        binds[i].buffer = buffers[i].data();
        binds[i].buffer_length = bufferSize;
        binds[i].is_null = reinterpret_cast<bool*>(&isNull[i]);
        binds[i].length = &lengths[i];
        binds[i].error = reinterpret_cast<bool*>(&error[i]);
    }
    if (mysql_stmt_bind_result(stmt, binds.get())) {
        throw DatabaseException("Failed to bind result in fetchFieldsByCondition: " +
            std::string(mysql_stmt_error(stmt)));
    }

    // 存储结果
    if (mysql_stmt_store_result(stmt)) {
        throw DatabaseException("Failed to store result in fetchFieldsByCondition: " +
            std::string(mysql_stmt_error(stmt)));
    }

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
            if (isNull[i]) {
                rowData[fieldNames[i]] = {};
            }
            else {
                rowData[fieldNames[i]] = std::string(buffers[i].data(), lengths[i]);
            }
        }
        results.push_back(std::move(rowData));
    }

    return results;
}

template<typename Entity, typename ID_TYPE>
template<typename... Args>
inline void FKBaseMapper<Entity, ID_TYPE>::bindValues(MySQLStmtPtr& stmtPtr, const Args&... args) const
{
    MYSQL_STMT* stmt = stmtPtr;
    constexpr size_t count = sizeof...(Args);
    std::vector<MYSQL_BIND> binds(count, MYSQL_BIND{});

    // 逐个绑定参数
    bindParams(binds.data(), args...);

    if (mysql_stmt_bind_param(stmt, binds.data())) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

template<typename Entity, typename ID_TYPE>
template<typename ValueType>
void FKBaseMapper<Entity, ID_TYPE>::bindSingleValue(MYSQL_BIND& bind, const ValueType& value) const
{
    using Type = std::remove_cv_t<std::remove_reference_t<ValueType>>;
    //using Type = std::decay_t<ValueType>; // 类型退化、数组→指针转换、函数→函数指针
    if constexpr (std::is_integral_v<Type>) {
        if constexpr (std::is_same_v<Type, bool>) {
            bind.buffer_type = MYSQL_TYPE_TINY;
        }
        else if constexpr (sizeof(Type) <= 1) {
            bind.buffer_type = MYSQL_TYPE_TINY;
        }
        else if constexpr (sizeof(Type) <= 2) {
            bind.buffer_type = MYSQL_TYPE_SHORT;
        }
        else if constexpr (sizeof(Type) <= 4) {
            bind.buffer_type = MYSQL_TYPE_LONG;
        }
        else {
            bind.buffer_type = MYSQL_TYPE_LONGLONG;
        }

        bind.is_unsigned = std::is_unsigned_v<Type>;
        bind.buffer = const_cast<Type*>(&value);
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (std::is_floating_point_v<Type>) {
        if constexpr (std::is_same_v<Type, float>) {
            bind.buffer_type = MYSQL_TYPE_FLOAT;
        }
        else {
            bind.buffer_type = MYSQL_TYPE_DOUBLE;
        }

        bind.buffer = const_cast<Type*>(&value);
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (is_varchar_type_v<Type>) {
        bind.buffer_type = MYSQL_TYPE_VAR_STRING;
        bind.buffer = const_cast<char*>(value.val.c_str());
        bind.buffer_length = value.len;
        bind.length = &value.len;
        bind.is_null = 0;
    }
    else if constexpr (std::is_same_v<Type, MYSQL_TIME>) {
        bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
        bind.buffer = (void*)&value;
        bind.buffer_length = sizeof(value);
        bind.is_null = 0;
        bind.length = 0;
    }
    else if constexpr (is_optional_type_v<Type>) {
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
