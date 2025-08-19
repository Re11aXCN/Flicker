/*************************************************************************************
 *
 * @ Filename     : BaseMapper.hpp
 * @ Description : 基础数据库映射器模板类，提供通用的数据库操作
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/22
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef UNIVERSAL_BASE_MAPPER_HPP_
#define UNIVERSAL_BASE_MAPPER_HPP_

#include <string>
#include <vector>
#include <any>
#include <unordered_map>
#include <ranges>
#include <optional>
#include <chrono>
#include <memory>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <mysql.h>

#include "cxx_utils.h"
#include "connection_pool.h"
#include "field_mapper.hpp"
//< Use std::chrono::to convert the current time to the MYSQL_TIME type
// 
//< Convert MYSQL_TIME to std::chrono::system_clock::time_point,
//  only MYSQL_TIMESTAMP_DATETIME and MYSQL_TIMESTAMP_DATE conversions are supported
//  other types will throw std::runtime_error , /*Future maybe support convert to std::string?*/
#include "time.h"

//< {StmtPtr}:
//   MySQL statement intelligent pointer for automatic management of MYSQL_STMT resources
//   destructors will automatically call "mysql_stmt_close"
// 
//< {ResPtr}:
//   ResPtr is a RAII class for automatic management of MYSQL_RES resources
//   destructors will automatically call "mysql_free_result"
// 
//< {ResultSetParser}:
//   ResultSetParser will parse the data in memory after the output MYSQL_BIND or
//   fetch MYSQL_ROW after the mysql_query execution or mysql_stmt_execute execution,
//   and then parse the data in memory to obtain the C++ data type
//
// 
//   For details, please read the function of document browsing
#include "stmt_ptr.h"
#include "res_ptr.h"
#include "result_set_parser.h"

//< Provide value query conditions, such as =, <>, >=, >, <, <=,
//  provide range conditions, BETWEEN AND,
//  provide multi - conditional combinations, AND, OR, NOT,
//  provide collection operations, IN, NOT IN,
//  provide fuzzy queries, LIKE, REGEXP,
//  provide null value check, IS NULL, IS NOT NULL,
//  provide the extension of native SQL fragment queries
//
//< You can use {QueryConditionBuilder} for quick construction
#include "query_condition.h"

#define MAX_ROW_SIZE_U64 18446744073709551615  

namespace universal::mysql {

template<typename Entity, typename ID_TYPE>
class BaseMapper {
protected:
    ConnectionPool* _pool;
    typename IntrospectiveFieldMapper<Entity>::VariantMap _fieldMappings;
    ResultSetParser _parser; // 结果集解析器
public:
    using EntityRows = std::vector<Entity>;
    using FieldMap = std::unordered_map<std::string, std::any>;
    using FieldMapRows = std::vector<FieldMap>;
    explicit BaseMapper(ConnectionPool* pool)
        : _pool(pool)
    {
        IntrospectiveFieldMapper<Entity> mapper;
        mapper.populateMap(_fieldMappings);
    }
    virtual ~BaseMapper() = default;

    BaseMapper(const BaseMapper&) = delete;
    BaseMapper& operator=(const BaseMapper&) = delete;

    BaseMapper(BaseMapper&&) = delete;
    BaseMapper& operator=(BaseMapper&&) = delete;
    
    const auto& getFieldMappings() const;
    template<typename ValueType>
    std::optional<ValueType> getFieldValue(const std::string& fieldName) const;

    std::optional<Entity> findById(ID_TYPE id);
    MySQLResult<EntityRows> findAll();
    MySQLResult<my_ulonglong> insert(const Entity& entity);
    MySQLResult<my_ulonglong> deleteById(ID_TYPE id);

    // 安全的字段更新方法 - 使用字段名称列表和对应的值来防止SQL注入
    // 示例用法: updateFieldsByIdSafe(id, {"field1", "field2"}, value1, value2);
    template<typename... Args>
    MySQLResult<my_ulonglong> updateFieldsById(ID_TYPE id, const std::vector<std::string>& fieldNames, Args&&... args);

    // 通用的查询实体方法，支持排序和分页
    template<typename... Args >
    MySQLResult<EntityRows> queryEntities(const std::string& sql, Args&&... args) const;

    // 使用查询条件策略的实体查询方法
    template<bool Distinct = false, SortOrder Order = SortOrder::Ascending, utils::string::fixed_string FieldName = "id",
        Pagination pagination = Pagination{ 0, 0 } >
    MySQLResult<EntityRows> queryEntitiesByCondition(const std::unique_ptr<IQueryCondition>& condition) const;

    // 使用查询条件策略的更新方法
    template<typename... Args>
    MySQLResult<my_ulonglong> updateFieldsByCondition(const std::unique_ptr<IQueryCondition>& condition,
        const std::vector<std::string>& fieldNames,
        Args&&... args) const;
    // 使用查询条件策略的通用查询方法
    template<bool Distinct = false, SortOrder Order = SortOrder::Ascending, utils::string::fixed_string FieldName = "id",
        Pagination pagination = Pagination{ 0, 0 }>
    MySQLResult<FieldMapRows> queryFieldsByCondition(
        const std::unique_ptr<IQueryCondition>& condition,
        const std::vector<std::string>& fieldNames) const;

    // 使用查询条件策略的计数方法
    template<bool Distinct = false>
    MySQLResult<my_ulonglong> countByCondition(const std::unique_ptr<IQueryCondition>& condition) const;

    // 使用查询条件策略的删除方法
    MySQLResult<my_ulonglong> deleteByCondition(const std::unique_ptr<IQueryCondition>& condition) const;
    MySQLResult<my_ulonglong> truncateTable(bool confirm = false) const;

    MySQLVoidResult createTable();
    MySQLVoidResult dropTable(bool confirm = false);

    enum class GetFieldMapResStatus {
        Success,
        BadCast,
        NotFound//OutOfRange,
    };
    template<typename ValueType>
    auto getFieldMapValue(const FieldMap& fieldMap, const std::string& fieldName) const -> std::pair<std::optional<ValueType>, GetFieldMapResStatus>;
    template<typename ValueType>
    ValueType getFieldMapValueOrDefault(const FieldMap& fieldMap, const std::string& fieldName,
        ValueType defaultValue) const;
protected:
    // 子类必须实现的查询方法
    virtual constexpr std::string getTableName() const = 0;
    virtual constexpr std::string createTableQuery() const = 0;
    virtual constexpr std::string findByIdQuery() const = 0;
    virtual constexpr std::string findAllQuery() const = 0;
    virtual constexpr std::string insertQuery() const = 0;
    virtual constexpr std::string deleteByIdQuery() const = 0;

    // 子类必须实现的参数绑定方法
    virtual bool bindInsertParams(StmtPtr& stmtPtr, const Entity& entity) const = 0;

    // 子类必须实现的结果处理方法
    virtual Entity createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
        char* isNulls, size_t columnCount) const = 0;
    virtual Entity createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields, 
        unsigned long* lengths, size_t columnCount) const = 0;
    
    // 整形、浮点型、mysql_varchar、MYSQL_TIME、std::optional参数绑定方法
    template<typename... Args>
    MySQLVoidResult bindValues(StmtPtr& stmtPtr, Args&&... args) const;
    template<typename... Args>
    MySQLVoidResult bindValues(StmtPtr& stmtPtr, MYSQL_BIND* existingBinds, size_t& startIndex, Args&&... args) const;
    MySQLVoidResult bindConditionValues(StmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition) const;
    MySQLVoidResult bindConditionValues(StmtPtr& stmtPtr, const std::unique_ptr<IQueryCondition>& condition, MYSQL_BIND* existingBinds, size_t& startIndex) const;

    MySQLResult<StmtPtr> prepareStatement(const std::string& query) const;
    MySQLVoidResult executeQuery(StmtPtr& stmtPtr) const;

    // 编译时构建UPDATE SET子句和绑定值tuple,使用BindableParam/ExpressionParam, 详细见"Inl-CoreStructDef.ipp"
    template<typename... Args, size_t... Is>
    auto buildSetClauseAndBindTuple(const std::vector<std::string>& fieldNames,
        std::index_sequence<Is...>,
        Args&&... args) const;

    template <bool Distinct>
    static constexpr auto buildDistinctClause();
    template <SortOrder Order, utils::string::fixed_string FieldName>
    static constexpr auto buildOrderClause();
    template <Pagination Pagination>
    static constexpr auto buildLimitClause();
private:
    MySQLResult<EntityRows> _find_all_impl(const std::string& query) const;
    MySQLResult<FieldMapRows> _query_fields_impl(const std::string& query) const;
    MySQLResult<my_ulonglong> _count_all_impl(const std::string& query) const;
    MySQLResult<my_ulonglong> _delete_all_impl(const std::string& query) const;
};

}

#ifdef UNIVERSAL_MYSQL_HEADER_ONLY
#include "base_mapper-impl.ipp"
#endif

#endif // !UNIVERSAL_BASE_MAPPER_HPP_
