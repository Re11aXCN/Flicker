/*************************************************************************************
 *
 * @ Filename     : FKBaseMapper.hpp
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
#ifndef FK_BASE_MAPPER_HPP_
#define FK_BASE_MAPPER_HPP_

#include <string>
#include <vector>
#include <ranges>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <memory>
#include <mutex>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <mysql.h>

#include "../FKMySQLConnectionPool.h"
#include "FKDef.h"
#include "FKLogger.h"
#include "FKUtils.h"
#include "FKFieldMapper.hpp"
#define MAX_ROW_SIZE 18446744073709551615  
// 数据库异常类
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message) : std::runtime_error(message) {}
};
// MySQL语句智能指针，用于自动管理MYSQL_STMT资源
class MySQLStmtPtr {
private:
    MYSQL_STMT* _stmt;
    bool _released;

public:
    explicit MySQLStmtPtr(MYSQL_STMT* stmt) : _stmt(stmt), _released(false) {}

    ~MySQLStmtPtr() {
        release();
    }

    // 禁止复制
    MySQLStmtPtr(const MySQLStmtPtr&) = delete;
    MySQLStmtPtr& operator=(const MySQLStmtPtr&) = delete;

    // 允许移动
    MySQLStmtPtr(MySQLStmtPtr&& other) noexcept : _stmt(other._stmt), _released(other._released) {
        other._stmt = nullptr;
        other._released = true;
    }

    MySQLStmtPtr& operator=(MySQLStmtPtr&& other) noexcept {
        if (this != &other) {
            release();
            _stmt = other._stmt;
            _released = other._released;
            other._stmt = nullptr;
            other._released = true;
        }
        return *this;
    }

    // 获取原始指针
    MYSQL_STMT* get() const {
        return _stmt;
    }

    // 释放资源
    void release() {
        if (_stmt && !_released) {
            mysql_stmt_close(_stmt);
            _released = true;
        }
        _stmt = nullptr;
    }

    // 转换为原始指针
    operator MYSQL_STMT* () const {
        return _stmt;
    }

    // 检查是否有效
    bool isValid() const {
        return _stmt != nullptr && !_released;
    }

    // 释放所有权（不关闭语句）
    MYSQL_STMT* release_ownership() {
        MYSQL_STMT* temp = _stmt;
        _stmt = nullptr;
        _released = true;
        return temp;
    }
};

struct ResultGuard {
    MYSQL_RES* res;
    ResultGuard(MYSQL_RES* r) : res(r) {}
    ~ResultGuard() { if (res) mysql_free_result(res); }
};

struct varchar {
    std::string val;
    unsigned long len;
};
template<typename Type>
struct is_varchar_type : std::false_type {};
template<>
struct is_varchar_type<varchar> : std::true_type {};
template<typename Type>
inline constexpr bool is_varchar_type_v = is_varchar_type<Type>::value;

template<typename Type>
struct is_optional_type : std::false_type {};
template<typename Type>
struct is_optional_type<std::optional<Type>> : std::true_type {};
template<typename Type>
inline constexpr bool is_optional_type_v = is_optional_type<Type>::value;

enum class SortOrder {
    Ascending,  // 升序
    Descending  // 降序
};

struct OrderBy {
    SortOrder order;
    const char* fieldName;
};

struct Pagination {
    size_t limit;
    size_t offset;
};

// 基础数据库映射器模板类
template<typename Entity, typename ID_TYPE>
class FKBaseMapper {
public:

    typename IntrospectiveFieldMapper<Entity>::VariantMap _fieldMappings;

    FKBaseMapper() {
        IntrospectiveFieldMapper<Entity> mapper;
        mapper.populateMap(_fieldMappings);
    }
    virtual ~FKBaseMapper() = default;

    FKBaseMapper(const FKBaseMapper&) = delete;
    FKBaseMapper& operator=(const FKBaseMapper&) = delete;

    FKBaseMapper(FKBaseMapper&&) = delete;
    FKBaseMapper& operator=(FKBaseMapper&&) = delete;
    
    const auto& getFieldMappings() const;
    template<typename ValueType>
    std::optional<ValueType> getFieldValue(const std::string& fieldName) const;

    MYSQL_TIME mysqlCurrentTime() const;

    std::optional<Entity> findById(ID_TYPE id);
    template <OrderBy orderBy = OrderBy{ SortOrder::Ascending, "id" },
        Pagination pagination = Pagination{ 0, 0 } >
    std::vector<Entity> findAll();
    DbOperator::Status insert(const Entity& entity);
    DbOperator::Status deleteById(ID_TYPE id);

    // 安全的字段更新方法 - 使用字段名称列表和对应的值来防止SQL注入
    // 示例用法: updateFieldsByIdSafe(id, {"field1", "field2"}, value1, value2);
    template<typename... Args>
    DbOperator::Status updateFieldsById(ID_TYPE id, const std::vector<std::string>& fieldNames, Args&&... args);

    // 通用的按条件查询指定字段方法
    template<typename ConditionType>
    std::vector<std::unordered_map<std::string, std::string>> fetchFieldsByCondition(
        const std::string& tableName,
        const std::vector<std::string>& fieldNames,
        const std::string& conditionField,
        const ConditionType& conditionValue) const;
protected:
    // 子类必须实现的查询方法
    virtual constexpr std::string getTableName() const = 0;
    virtual constexpr std::string findByIdQuery() const = 0;
    virtual constexpr std::string findAllQuery() const = 0;
    virtual constexpr std::string insertQuery() const = 0;
    virtual constexpr std::string deleteByIdQuery() const = 0;

    // 子类必须实现的参数绑定方法
    virtual void bindInsertParams(MySQLStmtPtr& stmtPtr, const Entity& entity) const = 0;

    // 子类必须实现的结果处理方法
    virtual Entity createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const = 0;

    MySQLStmtPtr prepareStatement(std::string query) const;
    void executeQuery(MySQLStmtPtr& stmtPtr) const;

    // 整形、浮点型、varchar、MYSQL_TIME、std::optional参数绑定方法
    template<typename... Args>
    void bindValues(MySQLStmtPtr& stmtPtr, const Args&... args) const;
    // 递归终止条件
    void bindParams(MYSQL_BIND*) const {}
    // 递归绑定参数
    template<typename ValueType, typename... Rest>
    void bindParams(MYSQL_BIND* binds, const ValueType& arg, const Rest&... rest) const {
        bindSingleValue(*binds, arg);
        bindParams(binds + 1, rest...);
    }
    // 单个值绑定
    template<typename ValueType>
    void bindSingleValue(MYSQL_BIND& bind, const ValueType& value) const;
    
    std::vector<Entity> fetchRows(MySQLStmtPtr& stmtPtr) const {
        MYSQL_STMT* stmt = stmtPtr;
        
        my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
        if (rowCount == 0) return {};

        ResultGuard resultGuard(mysql_stmt_result_metadata(stmt));
        if (!resultGuard.res) throw DatabaseException("No metadata available for fetchRows query");

        std::size_t columnCount = mysql_num_fields(resultGuard.res);
        auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
        auto isNull = std::make_unique<char[]>(columnCount);
        auto lengths = std::make_unique<unsigned long[]>(columnCount);
        auto error = std::make_unique<char[]>(columnCount);
        std::fill_n(isNull.get(), columnCount, 0);
        std::fill_n(error.get(), columnCount, 0);
        
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
            throw DatabaseException("Failed to bind result in fetchRows: " +
                std::string(mysql_stmt_error(stmt)));
        }

        if (mysql_stmt_store_result(stmt)) {
            throw DatabaseException("Failed to store result in fetchRows: " +
                std::string(mysql_stmt_error(stmt)));
        }
        
        // 准备用于创建实体的临时数据
        std::vector<const char*> rowData(columnCount);
        std::vector<Entity> results;
        results.reserve(rowCount);
        // 获取所有行的结果
        while (true) {
            int fetch_result = mysql_stmt_fetch(stmt);
            if (fetch_result == MYSQL_NO_DATA) {
                break; // 没有更多数据
            } else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {// 数据截断
                throw DatabaseException("Fetch failed in fetchRows with error code: " + std::to_string(fetch_result));
            }
            
            // 处理当前行的数据
            for (size_t i = 0; i < columnCount; i++) {
                if (isNull[i]) {
                    rowData[i] = {};
                } else {
                    rowData[i] = static_cast<char*>(buffers[i].data());
                }
            }
            
            // 使用子类实现的方法创建实体并添加到结果集
            results.push_back(createEntityFromRow(const_cast<char**>(rowData.data()), lengths.get()));
        }
        
        return results;
    }
private:
    template <OrderBy orderBy>
    static constexpr auto buildOrderByClause() {
        constexpr auto field = orderBy.fieldName;
        constexpr auto direction = (orderBy.order == SortOrder::Ascending) ? " ASC" : " DESC";

        return FKUtils::concat(" ORDER BY ", field, direction);
    }
    template <Pagination pagination>
    static constexpr auto buildLimitClause() {
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
};

#include "FKBaseMapper-inl.hpp"

#endif // !FK_BASE_MAPPER_HPP_
