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

struct StmtOutputRes {
    my_ulonglong rowCount{ 0 };
    my_ulonglong columnCount{ 0 };
    std::unique_ptr<MYSQL_BIND[]> binds{ nullptr };
    std::unique_ptr<char[]> isNull{ nullptr };
    std::unique_ptr<unsigned long[]> lengths{ nullptr };
    std::unique_ptr<char[]> error{ nullptr };
    std::vector<std::vector<char>> buffers{};
};

struct varchar {
    const char* val;
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

// 一个能在编译期构造的、固定长度的字符串类
template<size_t N>
struct fixed_string {
    std::array<char, N> data{};

    consteval fixed_string(const char(&str)[N]) {
        std::copy_n(str, N, data.begin());
    }

    auto operator<=>(const fixed_string&) const = default;

    constexpr size_t size() const { return N - 1; }
    constexpr const char* c_str() const { return data.data(); }
    constexpr operator std::string_view() const { return { data.data(), size() }; }
};

template<size_t N>
fixed_string(const char(&)[N]) -> fixed_string<N>;

template<size_t N>
struct OrderBy {
    SortOrder order;
    fixed_string<N + 1> fieldName; // +1 是为了包含'\0' 结尾符

    auto operator<=>(const OrderBy&) const = default;
};

struct Pagination {
    size_t limit;
    size_t offset;
};

// 基础数据库映射器模板类
template<typename Entity, typename ID_TYPE>
class FKBaseMapper {
    typename IntrospectiveFieldMapper<Entity>::VariantMap _fieldMappings;
public:
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
    template <SortOrder Order = SortOrder::Ascending, fixed_string FieldName = "id",
        Pagination pagination = Pagination{ 0, 0 } >
    std::vector<Entity> findAll();
    DbOperator::Status insert(const Entity& entity);
    DbOperator::Status deleteById(ID_TYPE id);

    // 安全的字段更新方法 - 使用字段名称列表和对应的值来防止SQL注入
    // 示例用法: updateFieldsByIdSafe(id, {"field1", "field2"}, value1, value2);
    template<typename... Args>
    DbOperator::Status updateFieldsById(ID_TYPE id, const std::vector<std::string>& fieldNames, Args&&... args);

    // 通用的按条件查询指定字段方法，支持排序和分页
    template<typename ConditionType,
        SortOrder Order = SortOrder::Ascending, fixed_string FieldName = "id",
        Pagination pagination = Pagination{ 0, 0 } >
    std::vector<std::unordered_map<std::string, std::string>> queryFieldsByCondition(
        const std::string& tableName,
        const std::vector<std::string>& fieldNames,
        const std::string& conditionField,
        const ConditionType& conditionValue) const;

    // 通用的查询实体方法，支持排序和分页
    template<SortOrder Order = SortOrder::Ascending, fixed_string FieldName = "id",
        Pagination pagination = Pagination{ 0, 0 },
        typename... Args>
    std::vector<Entity> queryEntities(std::string&& sql, Args&&... args) const;

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
    void bindValues(MySQLStmtPtr& stmtPtr, Args&&... args) const;
    // 使用折叠表达式替代递归
    template<typename... Args>
    void bindParams(MYSQL_BIND* binds, Args&&... args) const {
        size_t index = 0;
        (bindSingleValue(binds[index++], std::forward<Args>(args)), ...);
    }
    // 单个值绑定
    template<typename ValueType>
    void bindSingleValue(MYSQL_BIND& bind, ValueType&& value) const;

private:
    template <SortOrder Order, fixed_string FieldName>
    static constexpr auto _build_order_by_clause();
    template <Pagination pagination>
    static constexpr auto _build_limit_clause();

    void _process_stmt_result(MYSQL_STMT* stmt, StmtOutputRes& output) const;

};

#include "FKBaseMapper-inl.hpp"

#endif // !FK_BASE_MAPPER_HPP_
