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
#include <optional>
#include <chrono>
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

inline constexpr size_t MAX_ALLOWED_LIMIT = 5000; // 最大允许的查询结果数量
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
// 基础数据库映射器模板类
template<typename T, typename ID_TYPE>
class FKBaseMapper {
public:
    // 辅助模板：检测是否为std::optional类型
    template<typename T>
    struct is_optional : std::false_type {};

    template<typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    // 辅助模板：检测是否为std::vector类型
    template<typename T>
    struct is_vector : std::false_type {};

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : std::true_type {};

    virtual ~FKBaseMapper() = default;
    
    // 获取字段映射
    const auto& getFieldMappings() const {
        return _fieldMappings;
    }
    
    // 获取指定字段的值
    template<typename ValueType>
    std::optional<ValueType> getFieldValue(const std::string& fieldName) const {
        if(_fieldMappings.contains(fieldName)){
             return std::get<ValueType>(_fieldMappings.at(fieldName));
        }
        return std::nullopt;
    }
    
    // 初始化字段映射
    void initializeFieldMappings() {
        IntrospectiveFieldMapper<T> mapper;
        mapper.populateMap(_fieldMappings);
    }
    
    // 根据id查找单条记录
    std::optional<T> findById(ID_TYPE id) {
        try {
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(findByIdQuery());
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for findById");
            }
            
            // 绑定参数
            bindIdParam(stmtPtr, &id);
            
            // 执行查询
            executeQuery(stmtPtr);
            
            // 获取元数据
            MYSQL_RES* meta = mysql_stmt_result_metadata(stmtPtr);
            if (!meta) {
                throw DatabaseException("No metadata available for findById query");
            }
            
            // 使用智能指针确保meta资源被释放
            struct MetaGuard {
                MYSQL_RES* res;
                ~MetaGuard() { if(res) mysql_free_result(res); }
            } metaGuard{meta};
            
            // 直接使用返回std::optional的fetchSingleResult方法
            return fetchSingleResult(stmtPtr, meta);
            
            // 不需要手动释放资源，智能指针和守卫会自动处理
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("findById error: {}", e.what()));
            throw;
        }
    }
    
    // 查找所有记录
    std::vector<T> findAll() {
        try {
            return FKMySQLConnectionPool::getInstance()->executeWithConnection(
                [this](MYSQL* mysql) -> std::vector<T> {
                    if (mysql_query(mysql, findAllQuery().c_str())) {
                        throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
                    }
                    
                    MYSQL_RES* res = mysql_store_result(mysql);
                    if (!res) {
                        throw DatabaseException("Store result failed");
                    }
                    
                    return processResultSet(res);
                }
            );
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("findAll error: {}", e.what()));
            throw;
        }
    }
    
    // 查找指定范围的记录（根据ID排序）
    std::vector<T> findAll(size_t limit, size_t offset = 0) {
        try {
            size_t safe_limit = limit > MAX_ALLOWED_LIMIT ? MAX_ALLOWED_LIMIT : limit;
            std::string query = FKUtils::concat(findAllQuery(), " LIMIT ", std::to_string(safe_limit), 
                               " OFFSET ", std::to_string(offset));
            
            return FKMySQLConnectionPool::getInstance()->executeWithConnection(
                [&query, this](MYSQL* mysql) -> std::vector<T> {
                    if (mysql_query(mysql, query.c_str())) {
                        throw DatabaseException("Query failed: " + std::string(mysql_error(mysql)));
                    }
                    
                    MYSQL_RES* res = mysql_store_result(mysql);
                    if (!res) {
                        throw DatabaseException("Store result failed");
                    }
                    
                    return processResultSet(res);
                }
            );
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("findAll with limit error: {}", e.what()));
            throw;
        }
    }
    
    // 插入记录
    DbOperator::Status insert(const T& entity) {
        try {
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(insertQuery());
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for insert");
            }
            
            // 绑定参数
            bindInsertParams(stmtPtr, entity);
            
            // 执行更新
            executeUpdate(stmtPtr);
            
            // 资源会由智能指针自动清理
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            std::string error = e.what();
            FK_SERVER_ERROR(std::format("insert error: {}", error));
            
            // 检查是否是重复键错误
            if (error.find("Duplicate entry") != std::string::npos) {
                return DbOperator::Status::DataAlreadyExist;
            }
            
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 根据ID删除记录
    DbOperator::Status deleteById(ID_TYPE id) {
        try {
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(deleteByIdQuery());
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for deleteById");
            }
            
            // 绑定参数
            bindIdParam(stmtPtr, &id);
            
            // 执行更新
            executeUpdate(stmtPtr);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
            
            // 资源会由智能指针自动清理
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("deleteById error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 根据ID更新指定字段 - 不安全的方法，仅用于内部受信任的调用
    // 注意：fields参数必须是预先定义好的字段名和占位符，如 "field1 = ?, field2 = ?"
    // 不能包含用户输入的未经验证的内容，以防止SQL注入
    // 
    // 安全替代方案：
    // 1. 使用 updateFieldsByIdSafe(id, {"field1", "field2"}, value1, value2) 方法
    // 2. 使用 updateFieldsByIdMap(id, {{"field1", value1}, {"field2", value2}}) 方法
    template<typename... Args>
    [[deprecated("This method is vulnerable to SQL injection. Use updateFieldsByIdSafe or updateFieldsByIdMap instead.")]]
    DbOperator::Status updateFieldsById(ID_TYPE id, const std::string& fields, Args&&... args) {
        try {
            // 安全检查：确保fields参数不包含可能导致SQL注入的字符
            if (fields.find(';') != std::string::npos || 
                fields.find('"') != std::string::npos || 
                fields.find('\'') != std::string::npos || 
                fields.find('--') != std::string::npos) {
                throw DatabaseException("Invalid fields parameter: potential SQL injection detected");
            }
            
            std::string query = FKUtils::concat("UPDATE ", getTableName(), " SET ", fields,  
                               " WHERE id = ?");
            
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(query);
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for updateFieldsById");
            }
            
            // 绑定参数
            bindUpdateParams(stmtPtr, id, std::forward<Args>(args)...);
            
            // 执行更新
            executeUpdate(stmtPtr);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
            
            // 资源会由智能指针自动清理
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("updateFieldsById error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 安全的字段更新方法 - 使用字段名称列表和对应的值来防止SQL注入
    // 示例用法: updateFieldsByIdSafe(id, {"field1", "field2"}, value1, value2);
    template<typename... Args>
    DbOperator::Status updateFieldsByIdSafe(ID_TYPE id, const std::vector<std::string>& fieldNames, Args&&... args) {
        try {
            // 参数数量检查
            if (sizeof...(args) != fieldNames.size()) {
                throw DatabaseException("Number of field names must match number of values");
            }
            
            // 构建安全的SET子句
            std::string setClause;
            for (size_t i = 0; i < fieldNames.size(); ++i) {
                if (i > 0) setClause += ", ";
                setClause += fieldNames[i] + " = ?";
            }
            
            std::string query = FKUtils::concat("UPDATE ", getTableName(), " SET ", setClause,
                " WHERE id = ?");
            
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(query);
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for updateFieldsByIdSafe");
            }
            
            // 绑定参数
            bindUpdateParams(stmtPtr, id, std::forward<Args>(args)...);
            
            // 执行更新
            executeUpdate(stmtPtr);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
            
            // 资源会由智能指针自动清理
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("updateFieldsByIdSafe error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 使用字段名和值的映射进行安全更新 - 更直观的API
    // 示例用法: updateFieldsByIdMap(id, {{"field1", value1}, {"field2", value2}});
    template<typename ValueType>
    DbOperator::Status updateFieldsByIdMap(ID_TYPE id, const std::map<std::string, ValueType>& fieldMap) {
        try {
            if (fieldMap.empty()) {
                return DbOperator::Status::Success; // 没有字段需要更新
            }
            
            // 构建安全的SET子句和值数组
            std::string setClause;
            std::vector<ValueType> values;
            
            bool first = true;
            for (const auto& [field, value] : fieldMap) {
                if (!first) setClause += ", ";
                setClause += field + " = ?";
                values.push_back(value);
                first = false;
            }
            
            std::string query = FKUtils::concat("UPDATE ", getTableName(), " SET ", setClause,
                " WHERE id = ?");
            
            // 使用智能指针管理MYSQL_STMT资源
            MySQLStmtPtr stmtPtr = prepareStatement(query);
            if (!stmtPtr.isValid()) {
                throw DatabaseException("Failed to prepare statement for updateFieldsByIdMap");
            }
            
            // 绑定参数
            bindMapValues(stmtPtr, values, id);
            
            // 执行更新
            executeUpdate(stmtPtr);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmtPtr);
            
            // 资源会由智能指针自动清理
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("updateFieldsByIdMap error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 绑定字段值向量和ID到MySQL语句参数
    template<typename ValueType>
    void bindMapValues(MySQLStmtPtr& stmtPtr, const std::vector<ValueType>& values, ID_TYPE id) const {
        MYSQL_STMT* stmt = stmtPtr.get();
        // 计算参数总数：字段值数量 + 1 (ID)
        size_t paramCount = values.size() + 1;
        
        // 创建MYSQL_BIND数组
        std::vector<MYSQL_BIND> binds(paramCount);
        memset(binds.data(), 0, sizeof(MYSQL_BIND) * paramCount);
        
        // 绑定字段值
        for (size_t i = 0; i < values.size(); ++i) {
            bindSingleValue(binds[i], values[i]);
        }
        
        // 绑定ID参数（最后一个参数）
        size_t idIndex = values.size();
        ID_TYPE idCopy = id; // 创建副本避免const_cast
        
        if constexpr (std::is_same_v<ID_TYPE, int> || std::is_same_v<ID_TYPE, long>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONG;
            binds[idIndex].is_unsigned = false;
        } else if constexpr (std::is_same_v<ID_TYPE, unsigned int> || std::is_same_v<ID_TYPE, unsigned long> || std::is_same_v<ID_TYPE, std::size_t>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONGLONG;
            binds[idIndex].is_unsigned = true;
        } else if constexpr (std::is_same_v<ID_TYPE, std::string>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_VAR_STRING;
            binds[idIndex].buffer = const_cast<char*>(idCopy.c_str());
            binds[idIndex].buffer_length = static_cast<unsigned long>(idCopy.length()) + 1;
            unsigned long length = static_cast<unsigned long>(idCopy.length());
            binds[idIndex].length = &length;
        } else {
            throw std::runtime_error("Unsupported ID type in bindMapValues");
        }
        
        if (!std::is_same_v<ID_TYPE, std::string>) {
            binds[idIndex].buffer = &idCopy;
            binds[idIndex].is_null = 0;
            binds[idIndex].length = 0;
        }
        
        // 绑定所有参数
        if (mysql_stmt_bind_param(stmt, binds.data())) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind param failed: " + error);
        }
    }
    
    // 绑定字段值映射和ID到MySQL语句参数
    template<typename ValueType>
    void bindMapValues(MySQLStmtPtr& stmt, const std::map<std::string, ValueType>& fieldsMap, int id) const {
        // 计算参数总数：字段数量 + 1 (ID)
        size_t paramCount = fieldsMap.size() + 1;
        
        // 创建MYSQL_BIND数组
        std::vector<MYSQL_BIND> binds(paramCount);
        memset(binds.data(), 0, sizeof(MYSQL_BIND) * paramCount);
        
        // 绑定字段值
        size_t index = 0;
        for (const auto& [field, value] : fieldsMap) {
            bindSingleValue(binds[index], value);
            index++;
        }
        
        // 绑定ID参数（最后一个参数）
        binds[index].buffer_type = MYSQL_TYPE_LONG;
        binds[index].buffer = const_cast<int*>(&id);
        binds[index].is_null = 0;
        binds[index].length = 0;
        
        // 将所有参数绑定到语句
        if (mysql_stmt_bind_param(stmt.get(), binds.data()) != 0) {
            throw DatabaseException("Failed to bind parameters in bindMapValues: " + 
                                   std::string(mysql_stmt_error(stmt.get())));
        }
    }
    
    // 通用的按条件查询指定字段方法
    template<typename Entity, typename ConditionType>
    std::vector<std::map<std::string, std::string>> fetchFieldsByCondition(
        const std::string& tableName,
        const std::vector<std::string>& fields,
        const std::string& conditionField,
        const ConditionType& conditionValue) const {
        
        // 构建字段列表
        std::string fieldList;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) fieldList += ", ";
            fieldList += fields[i];
        }
        
        // 构建SQL查询
        std::string sql = "SELECT " + fieldList + " FROM " + tableName + 
                         " WHERE " + conditionField + " = ?";
        
        try {
            // 准备预处理语句
            MySQLStmtPtr stmt = prepareStatement(sql);
            if (!stmt) {
                throw DatabaseException("Failed to prepare statement in fetchFieldsByCondition");
            }
            
            // 绑定条件参数
            MYSQL_BIND bind;
            memset(&bind, 0, sizeof(bind));
            bindSingleValue(bind, conditionValue);
            
            if (mysql_stmt_bind_param(stmt.get(), &bind) != 0) {
                throw DatabaseException("Failed to bind parameter in fetchFieldsByCondition: " + 
                                       std::string(mysql_stmt_error(stmt.get())));
            }
            
            // 执行查询
            if (mysql_stmt_execute(stmt.get()) != 0) {
                throw DatabaseException("Failed to execute query in fetchFieldsByCondition: " + 
                                       std::string(mysql_stmt_error(stmt.get())));
            }
            
            // 准备结果绑定
            std::vector<MYSQL_BIND> resultBinds(fields.size());
            memset(resultBinds.data(), 0, sizeof(MYSQL_BIND) * fields.size());
            
            // 为每个字段分配缓冲区
            const size_t bufferSize = 1024; // 假设每个字段最大1024字节
            std::vector<std::vector<char>> buffers(fields.size(), std::vector<char>(bufferSize));
            auto isNull = std::make_unique<char[]>(fields.size());
            auto lengths = std::make_unique<unsigned long[]>(fields.size());
            auto error = std::make_unique<char[]>(fields.size());

            // 初始化数组
            std::fill_n(isNull.get(), fields.size(), 0);
            std::fill_n(error.get(), fields.size(), 0);
            
            // 设置结果绑定
            for (size_t i = 0; i < fields.size(); ++i) {
                resultBinds[i].buffer_type = MYSQL_TYPE_STRING;
                resultBinds[i].buffer = buffers[i].data();
                resultBinds[i].buffer_length = bufferSize - 1;
                resultBinds[i].is_null = reinterpret_cast<bool*>(&isNull[i]);
                resultBinds[i].length = &lengths[i];
                resultBinds[i].error = reinterpret_cast<bool*>(&error[i]);
            }
            
            // 绑定结果集
            if (mysql_stmt_bind_result(stmt.get(), resultBinds.data()) != 0) {
                throw DatabaseException("Failed to bind result in fetchFieldsByCondition: " + 
                                       std::string(mysql_stmt_error(stmt.get())));
            }
            
            // 存储结果
            if (mysql_stmt_store_result(stmt.get()) != 0) {
                throw DatabaseException("Failed to store result in fetchFieldsByCondition: " + 
                                       std::string(mysql_stmt_error(stmt.get())));
            }
            
            // 获取结果
            std::vector<std::map<std::string, std::string>> results;
            while (mysql_stmt_fetch(stmt.get()) == 0) {
                std::map<std::string, std::string> row;
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (isNull[i]) {
                        row[fields[i]] = "NULL";
                    } else {
                        // 确保字符串以null结尾
                        buffers[i][lengths[i]] = '\0';
                        row[fields[i]] = std::string(buffers[i].data(), lengths[i]);
                    }
                }
                results.push_back(row);
            }
            
            return results;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("fetchFieldsByCondition error: {}", e.what()));
            throw;
        }
    }

protected:
    typename IntrospectiveFieldMapper<T>::VariantMap _fieldMappings;
    
    // 构造函数
    FKBaseMapper() {
        // 初始化字段映射
        initializeFieldMappings();
    }
    
    // 子类必须实现的查询方法
    virtual constexpr std::string getTableName() const = 0;
    virtual constexpr std::string findByIdQuery() const = 0;
    virtual constexpr std::string findAllQuery() const = 0;
    virtual constexpr std::string insertQuery() const = 0;
    virtual constexpr std::string deleteByIdQuery() const = 0;
    
    // 子类必须实现的参数绑定方法
    virtual void bindIdParam(MySQLStmtPtr& stmtPtr, ID_TYPE* id) const = 0;
    virtual void bindInsertParams(MySQLStmtPtr& stmtPtr, const T& entity) const = 0;
    
    // 子类必须实现的结果处理方法
    virtual T createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const = 0;
    
    // 更新参数绑定方法（变参模板）
    template<typename... Args>
    void bindUpdateParams(MySQLStmtPtr& stmtPtr, ID_TYPE id, Args&&... args) const {
        MYSQL_STMT* stmt = stmtPtr;
        // 计算参数总数（字段值 + ID）
        constexpr size_t paramCount = sizeof...(Args) + 1;
        
        // 创建绑定数组
        std::vector<MYSQL_BIND> binds(paramCount);
        memset(binds.data(), 0, sizeof(MYSQL_BIND) * paramCount);
        
        // 绑定字段值参数
        bindUpdateParamsImpl(binds, 0, std::forward<Args>(args)...);
        
        // 最后一个参数是ID
        size_t idIndex = paramCount - 1;
        ID_TYPE idCopy = id; // 创建副本避免const_cast
        
        if constexpr (std::is_same_v<ID_TYPE, int> || std::is_same_v<ID_TYPE, long>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONG;
            binds[idIndex].is_unsigned = false;
        } else if constexpr (std::is_same_v<ID_TYPE, unsigned int> || std::is_same_v<ID_TYPE, unsigned long> || std::is_same_v<ID_TYPE, std::size_t>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONGLONG;
            binds[idIndex].is_unsigned = true;
        } else if constexpr (std::is_same_v<ID_TYPE, std::string>) {
            // 处理字符串ID
            throw std::runtime_error("String ID not implemented in bindUpdateParams");
        } else {
            throw std::runtime_error("Unsupported ID type in bindUpdateParams");
        }
        
        binds[idIndex].buffer = &idCopy;
        binds[idIndex].is_null = 0;
        binds[idIndex].length = 0;
        
        // 绑定所有参数
        if (mysql_stmt_bind_param(stmt, binds.data())) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind param failed: " + error);
        }
    }
    
    // 绑定字段映射值和ID - 通用实现以支持updateFieldsByIdMap
    template<typename ValueType>
    void bindMapValues(MYSQL_STMT* stmt, const std::vector<ValueType>& values, ID_TYPE id) const {
        // 计算参数总数（字段值 + ID）
        size_t paramCount = values.size() + 1;
        
        // 创建绑定数组
        std::vector<MYSQL_BIND> binds(paramCount);
        memset(binds.data(), 0, sizeof(MYSQL_BIND) * paramCount);
        
        // 绑定字段值参数
        for (size_t i = 0; i < values.size(); ++i) {
            bindSingleValue(binds[i], values[i]);
        }
        
        // 最后一个参数是ID
        size_t idIndex = paramCount - 1;
        ID_TYPE idCopy = id; // 创建副本避免const_cast
        
        if constexpr (std::is_same_v<ID_TYPE, int> || std::is_same_v<ID_TYPE, long>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONG;
            binds[idIndex].is_unsigned = false;
        } else if constexpr (std::is_same_v<ID_TYPE, unsigned int> || std::is_same_v<ID_TYPE, unsigned long> || std::is_same_v<ID_TYPE, std::size_t>) {
            binds[idIndex].buffer_type = MYSQL_TYPE_LONGLONG;
            binds[idIndex].is_unsigned = true;
        } else if constexpr (std::is_same_v<ID_TYPE, std::string>) {
            // 处理字符串ID
            binds[idIndex].buffer_type = MYSQL_TYPE_VAR_STRING;
            binds[idIndex].buffer = const_cast<char*>(idCopy.c_str());
            binds[idIndex].buffer_length = static_cast<unsigned long>(idCopy.length()) + 1;
            unsigned long length = static_cast<unsigned long>(idCopy.length());
            binds[idIndex].length = &length;
        } else {
            throw std::runtime_error("Unsupported ID type in bindMapValues");
        }
        
        if (!std::is_same_v<ID_TYPE, std::string>) {
            binds[idIndex].buffer = &idCopy;
            binds[idIndex].is_null = 0;
            binds[idIndex].length = 0;
        }
        
        // 绑定所有参数
        if (mysql_stmt_bind_param(stmt, binds.data())) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind param failed: " + error);
        }
    }

    // 绑定单个值到MYSQL_BIND结构体
    template<typename T>
    void bindSingleValue(MYSQL_BIND& bind, const T& value) const {
        if constexpr (std::is_integral_v<T>) {
            // 整数类型
            if constexpr (std::is_same_v<T, bool>) {
                bind.buffer_type = MYSQL_TYPE_TINY;
            }
            else if constexpr (sizeof(T) <= 1) {
                bind.buffer_type = MYSQL_TYPE_TINY;
            }
            else if constexpr (sizeof(T) <= 2) {
                bind.buffer_type = MYSQL_TYPE_SHORT;
            }
            else if constexpr (sizeof(T) <= 4) {
                bind.buffer_type = MYSQL_TYPE_LONG;
            }
            else {
                bind.buffer_type = MYSQL_TYPE_LONGLONG;
            }

            bind.is_unsigned = std::is_unsigned_v<T>;
            bind.buffer = const_cast<T*>(&value);
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (std::is_floating_point_v<T>) {
            // 浮点类型
            if constexpr (std::is_same_v<T, float>) {
                bind.buffer_type = MYSQL_TYPE_FLOAT;
            }
            else {
                bind.buffer_type = MYSQL_TYPE_DOUBLE;
            }

            bind.buffer = const_cast<T*>(&value);
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (std::is_convertible_v<T, std::string>) {
            // 字符串类型
            std::string* strValue = new std::string(value);
            char* buffer = new char[strValue->length() + 1];
            std::copy(strValue->begin(), strValue->end(), buffer);
            buffer[strValue->length()] = '\0';

            unsigned long* length = new unsigned long(static_cast<unsigned long>(strValue->length()));

            bind.buffer_type = MYSQL_TYPE_VAR_STRING;
            bind.buffer = buffer;
            bind.buffer_length = *length + 1;
            bind.length = length;
            bind.is_null = 0;
        }
        else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            // 时间点类型
            MYSQL_TIME* mysqlTime = new MYSQL_TIME();
            memset(mysqlTime, 0, sizeof(MYSQL_TIME));

            auto timeT = std::chrono::system_clock::to_time_t(value);
            auto duration = value.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

            // 线程安全的时间转换
#if defined(_WIN32) || defined(_WIN64)
            struct tm timeinfo;
            localtime_s(&timeinfo, &timeT);
#else
            struct tm timeinfo;
            localtime_r(&timeT, &timeinfo);
#endif

            mysqlTime->year = timeinfo.tm_year + 1900;
            mysqlTime->month = timeinfo.tm_mon + 1;
            mysqlTime->day = timeinfo.tm_mday;
            mysqlTime->hour = timeinfo.tm_hour;
            mysqlTime->minute = timeinfo.tm_min;
            mysqlTime->second = timeinfo.tm_sec;
            mysqlTime->second_part = millis * 1000;  // 毫秒转微秒
            mysqlTime->time_type = MYSQL_TIMESTAMP_DATETIME;

            bind.buffer_type = MYSQL_TYPE_DATETIME;
            bind.buffer = mysqlTime;
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (is_optional<T>::value) {
            // std::optional类型
            using ValueType = typename T::value_type;

            if (value.has_value()) {
                bindSingleValue(bind, *value);
            }
            else {
                char* is_null = new char(1);
                bind.buffer_type = MYSQL_TYPE_NULL;
                bind.is_null = is_null;
            }
        }
        else {
            throw std::runtime_error("Unsupported value type in bindSingleValue");
        }
    }
    
    //template<typename T>
    //void bindSingleValue(MYSQL_BIND& bind, const T& value) const {
    //    using ValueType = std::remove_cv_t<std::remove_reference_t<T>>;
    //    
    //    if constexpr (std::is_same_v<ValueType, bool>) {
    //        bool valueCopy = value;
    //        bind.buffer_type = MYSQL_TYPE_TINY;
    //        bind.buffer = &valueCopy;
    //        bind.is_unsigned = true;
    //    } 
    //    else if constexpr (std::is_integral_v<ValueType>) {
    //        ValueType valueCopy = value;
    //        
    //        if constexpr (sizeof(ValueType) <= 1) {
    //            bind.buffer_type = MYSQL_TYPE_TINY;
    //        } else if constexpr (sizeof(ValueType) <= 2) {
    //            bind.buffer_type = MYSQL_TYPE_SHORT;
    //        } else if constexpr (sizeof(ValueType) <= 4) {
    //            bind.buffer_type = MYSQL_TYPE_LONG;
    //        } else {
    //            bind.buffer_type = MYSQL_TYPE_LONGLONG;
    //        }
    //        
    //        bind.is_unsigned = std::is_unsigned_v<ValueType>;
    //        bind.buffer = &valueCopy;
    //    }
    //    else if constexpr (std::is_floating_point_v<ValueType>) {
    //        ValueType valueCopy = value;
    //        
    //        if constexpr (std::is_same_v<ValueType, float>) {
    //            bind.buffer_type = MYSQL_TYPE_FLOAT;
    //        } else {
    //            bind.buffer_type = MYSQL_TYPE_DOUBLE;
    //        }
    //        
    //        bind.buffer = &valueCopy;
    //    }
    //    else if constexpr (std::is_convertible_v<ValueType, std::string>) {
    //        std::string strValue = value;
    //        bind.buffer_type = MYSQL_TYPE_VAR_STRING;
    //        bind.buffer = const_cast<char*>(strValue.c_str());
    //        bind.buffer_length = static_cast<unsigned long>(strValue.length()) + 1;
    //        unsigned long length = static_cast<unsigned long>(strValue.length());
    //        bind.length = &length;
    //    }
    //    else if constexpr (std::is_same_v<ValueType, std::chrono::system_clock::time_point>) {
    //        // 处理时间点类型
    //        MYSQL_TIME mysqlTime;
    //        memset(&mysqlTime, 0, sizeof(mysqlTime));
    //        
    //        auto timeT = std::chrono::system_clock::to_time_t(value);
    //        auto duration = value.time_since_epoch();
    //        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
    //        
    //        // 线程安全的时间转换
    //        #if defined(_WIN32) || defined(_WIN64)
    //            struct tm timeinfo;
    //            localtime_s(&timeinfo, &timeT);
    //        #else
    //            struct tm timeinfo;
    //            localtime_r(&timeT, &timeinfo);
    //        #endif
    //        
    //        mysqlTime.year = timeinfo.tm_year + 1900;
    //        mysqlTime.month = timeinfo.tm_mon + 1;
    //        mysqlTime.day = timeinfo.tm_mday;
    //        mysqlTime.hour = timeinfo.tm_hour;
    //        mysqlTime.minute = timeinfo.tm_min;
    //        mysqlTime.second = timeinfo.tm_sec;
    //        mysqlTime.second_part = millis * 1000;  // 毫秒转微秒
    //        mysqlTime.time_type = MYSQL_TIMESTAMP_DATETIME;
    //        
    //        bind.buffer_type = MYSQL_TYPE_DATETIME;
    //        bind.buffer = &mysqlTime;
    //    }
    //    else {
    //        throw std::runtime_error("Unsupported value type in bindSingleValue");
    //    }
    //    
    //    if (!std::is_convertible_v<ValueType, std::string> && 
    //        !std::is_same_v<ValueType, std::chrono::system_clock::time_point>) {
    //        bind.is_null = 0;
    //        bind.length = 0;
    //    }
    //}
    
    // 递归处理变参绑定（终止条件）
    void bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index) const {
        // 终止递归的空实现
    }

    // 递归处理变参绑定（处理整数类型）
    template<typename T, typename... Rest>
    typename std::enable_if_t<std::is_integral_v<std::remove_reference_t<T>>>
    bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index, T&& value, Rest&&... rest) const {
        using ValueType = std::remove_reference_t<T>;
        ValueType valueCopy = value; // 创建副本避免const_cast
        
        if constexpr (std::is_same_v<ValueType, bool>) {
            binds[index].buffer_type = MYSQL_TYPE_TINY;
        } else if constexpr (sizeof(ValueType) <= 1) {
            binds[index].buffer_type = MYSQL_TYPE_TINY;
        } else if constexpr (sizeof(ValueType) <= 2) {
            binds[index].buffer_type = MYSQL_TYPE_SHORT;
        } else if constexpr (sizeof(ValueType) <= 4) {
            binds[index].buffer_type = MYSQL_TYPE_LONG;
        } else {
            binds[index].buffer_type = MYSQL_TYPE_LONGLONG;
        }
        
        binds[index].is_unsigned = std::is_unsigned_v<ValueType>;
        binds[index].buffer = &valueCopy;
        binds[index].is_null = 0;
        binds[index].length = 0;
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }
    
    // 递归处理变参绑定（处理浮点类型）
    template<typename T, typename... Rest>
    typename std::enable_if_t<std::is_floating_point_v<std::remove_reference_t<T>>>
    bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index, T&& value, Rest&&... rest) const {
        using ValueType = std::remove_reference_t<T>;
        ValueType valueCopy = value; // 创建副本避免const_cast
        
        if constexpr (std::is_same_v<ValueType, float>) {
            binds[index].buffer_type = MYSQL_TYPE_FLOAT;
        } else {
            binds[index].buffer_type = MYSQL_TYPE_DOUBLE;
        }
        
        binds[index].buffer = &valueCopy;
        binds[index].is_null = 0;
        binds[index].length = 0;
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }
    
    // 递归处理变参绑定（处理字符串类型）
    template<typename T, typename... Rest>
    typename std::enable_if_t<std::is_convertible_v<std::remove_reference_t<T>, std::string>>
    bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index, T&& value, Rest&&... rest) const {
        // 创建字符串副本以避免const_cast
        std::string strValue = value;
        
        // 使用动态分配的内存，确保在函数返回后仍然有效
        char* buffer = new char[strValue.length() + 1];
        std::copy(strValue.begin(), strValue.end(), buffer);
        buffer[strValue.length()] = '\0'; // 确保以null结尾
        
        unsigned long length = static_cast<unsigned long>(strValue.length());
        
        binds[index].buffer_type = MYSQL_TYPE_VAR_STRING;
        binds[index].buffer = buffer;
        binds[index].buffer_length = length + 1;
        binds[index].length = &length;
        binds[index].is_null = 0;
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }
    
    // 递归处理变参绑定（处理时间点类型）
    template<typename T, typename... Rest>
    typename std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, std::chrono::system_clock::time_point>>
    bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index, T&& value, Rest&&... rest) const {
        // 处理时间点类型
        auto timePoint = value;
        MYSQL_TIME* mysqlTime = new MYSQL_TIME();
        memset(mysqlTime, 0, sizeof(MYSQL_TIME));
        
        auto timeT = std::chrono::system_clock::to_time_t(timePoint);
        auto duration = timePoint.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
        
        // 线程安全的时间转换
        #if defined(_WIN32) || defined(_WIN64)
            struct tm timeinfo;
            localtime_s(&timeinfo, &timeT);
        #else
            struct tm timeinfo;
            localtime_r(&timeT, &timeinfo);
        #endif
        
        mysqlTime->year = timeinfo.tm_year + 1900;
        mysqlTime->month = timeinfo.tm_mon + 1;
        mysqlTime->day = timeinfo.tm_mday;
        mysqlTime->hour = timeinfo.tm_hour;
        mysqlTime->minute = timeinfo.tm_min;
        mysqlTime->second = timeinfo.tm_sec;
        mysqlTime->second_part = millis * 1000;  // 毫秒转微秒
        mysqlTime->time_type = MYSQL_TIMESTAMP_DATETIME;
        
        binds[index].buffer_type = MYSQL_TYPE_DATETIME;
        binds[index].buffer = mysqlTime;
        binds[index].is_null = 0;
        binds[index].length = 0;
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }
    
    // 递归处理变参绑定（处理std::optional类型）
    template<typename T, typename... Rest>
    typename std::enable_if_t<is_optional<std::remove_reference_t<T>>::value>
    bindUpdateParamsImpl(std::vector<MYSQL_BIND>& binds, size_t index, T&& value, Rest&&... rest) const {
        using OptionalType = std::remove_reference_t<T>;
        using ValueType = typename OptionalType::value_type;
        
        if (value.has_value()) {
            // 如果optional有值，则绑定实际值
            if constexpr (std::is_integral_v<ValueType>) {
                ValueType valueCopy = *value;
                
                if constexpr (std::is_same_v<ValueType, bool>) {
                    binds[index].buffer_type = MYSQL_TYPE_TINY;
                } else if constexpr (sizeof(ValueType) <= 1) {
                    binds[index].buffer_type = MYSQL_TYPE_TINY;
                } else if constexpr (sizeof(ValueType) <= 2) {
                    binds[index].buffer_type = MYSQL_TYPE_SHORT;
                } else if constexpr (sizeof(ValueType) <= 4) {
                    binds[index].buffer_type = MYSQL_TYPE_LONG;
                } else {
                    binds[index].buffer_type = MYSQL_TYPE_LONGLONG;
                }
                
                binds[index].is_unsigned = std::is_unsigned_v<ValueType>;
                binds[index].buffer = &valueCopy;
                binds[index].is_null = 0;
                binds[index].length = 0;
            } 
            else if constexpr (std::is_floating_point_v<ValueType>) {
                ValueType valueCopy = *value;
                
                if constexpr (std::is_same_v<ValueType, float>) {
                    binds[index].buffer_type = MYSQL_TYPE_FLOAT;
                } else {
                    binds[index].buffer_type = MYSQL_TYPE_DOUBLE;
                }
                
                binds[index].buffer = &valueCopy;
                binds[index].is_null = 0;
                binds[index].length = 0;
            }
            else if constexpr (std::is_convertible_v<ValueType, std::string>) {
                std::string strValue = *value;
                char* buffer = new char[strValue.length() + 1];
                std::copy(strValue.begin(), strValue.end(), buffer);
                buffer[strValue.length()] = '\0';
                
                unsigned long length = static_cast<unsigned long>(strValue.length());
                
                binds[index].buffer_type = MYSQL_TYPE_VAR_STRING;
                binds[index].buffer = buffer;
                binds[index].buffer_length = length + 1;
                binds[index].length = &length;
                binds[index].is_null = 0;
            }
            else if constexpr (std::is_same_v<ValueType, std::chrono::system_clock::time_point>) {
                auto timePoint = *value;
                MYSQL_TIME* mysqlTime = new MYSQL_TIME();
                memset(mysqlTime, 0, sizeof(MYSQL_TIME));
                
                auto timeT = std::chrono::system_clock::to_time_t(timePoint);
                auto duration = timePoint.time_since_epoch();
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
                
                // 线程安全的时间转换
                #if defined(_WIN32) || defined(_WIN64)
                    struct tm timeinfo;
                    localtime_s(&timeinfo, &timeT);
                #else
                    struct tm timeinfo;
                    localtime_r(&timeT, &timeinfo);
                #endif
                
                mysqlTime->year = timeinfo.tm_year + 1900;
                mysqlTime->month = timeinfo.tm_mon + 1;
                mysqlTime->day = timeinfo.tm_mday;
                mysqlTime->hour = timeinfo.tm_hour;
                mysqlTime->minute = timeinfo.tm_min;
                mysqlTime->second = timeinfo.tm_sec;
                mysqlTime->second_part = millis * 1000;  // 毫秒转微秒
                mysqlTime->time_type = MYSQL_TIMESTAMP_DATETIME;
                
                binds[index].buffer_type = MYSQL_TYPE_DATETIME;
                binds[index].buffer = mysqlTime;
                binds[index].is_null = 0;
                binds[index].length = 0;
            }
            else {
                throw std::runtime_error("Unsupported optional value type in bindUpdateParamsImpl");
            }
        } else {
            // 如果optional没有值，则绑定NULL
            char is_null = 1;
            binds[index].buffer_type = MYSQL_TYPE_NULL;
            binds[index].is_null = &is_null;
        }
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }

    // 准备预处理语句
    MySQLStmtPtr prepareStatement(std::string query) const {
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
    
    // 执行查询
    void executeQuery(MySQLStmtPtr& stmtPtr) const {
        MYSQL_STMT* stmt = stmtPtr;
        if (mysql_stmt_execute(stmt)) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Execute failed: " + error);
        }
    }
    
    // 执行更新
    void executeUpdate(MySQLStmtPtr& stmtPtr) const {
        executeQuery(stmtPtr); // 执行逻辑相同
    }
    
    // 获取查询结果，可能是单行或多行
    // 返回std::vector<T>，如果没有结果则返回空向量
    // 获取单个结果，使用std::optional避免异常
    std::optional<T> fetchSingleResult(MySQLStmtPtr& stmtPtr, MYSQL_RES* meta) const {
        auto results = fetchRows(stmtPtr, meta);
        if (results.empty()) {
            return std::nullopt;
        }
        return results[0];
    }
    
    // 获取查询结果，可能是单行或多行
    // 返回std::vector<T>，如果没有结果则返回空向量
    std::vector<T> fetchRows(MySQLStmtPtr& stmtPtr, MYSQL_RES* meta) const {
        MYSQL_STMT* stmt = stmtPtr;
        if (mysql_stmt_store_result(stmt)) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Store result failed: " + error);
        }
        
        my_ulonglong rowCount = mysql_stmt_num_rows(stmt);
        if (rowCount == 0) return {};
        
        std::vector<T> results;
        results.reserve(rowCount);
        
        // 获取字段数量
        int columnCount = mysql_num_fields(meta);
        
        // 使用智能指针管理内存，确保资源正确释放
        auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
        auto isNull = std::make_unique<char[]>(columnCount);
        auto lengths = std::make_unique<unsigned long[]>(columnCount);
        auto error = std::make_unique<char[]>(columnCount);
        
        // 初始化数组
        std::fill_n(isNull.get(), columnCount, 0);
        std::fill_n(error.get(), columnCount, 0);
        
        // 分配内存用于存储结果 - 使用vector直接管理内存
        std::vector<std::vector<char>> buffers(columnCount);
        for (int i = 0; i < columnCount; i++) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(meta, i);

            unsigned long bufferSize = (field->type == MYSQL_TYPE_VAR_STRING || field->type == MYSQL_TYPE_STRING)
                ? field->length >> 2 : field->length;
            buffers[i].resize(bufferSize + 1);
            
            memset(&binds[i], 0, sizeof(MYSQL_BIND));
            binds[i].buffer_type = mysql_fetch_field_direct(meta, i)->type;
            binds[i].buffer = buffers[i].data();
            binds[i].buffer_length = bufferSize;
            binds[i].is_null = reinterpret_cast<bool*>(&isNull[i]);
            binds[i].length = &lengths[i];
            binds[i].error = reinterpret_cast<bool*>(&error[i]);
        }
        
        // 绑定结果
        if (mysql_stmt_bind_result(stmt, binds.get())) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Bind result failed: " + error);
        }
        
        // 准备用于创建实体的临时数据
        std::vector<const char*> rowData(columnCount);
        std::vector<unsigned long> rowLengths(columnCount);
        
        // 获取所有行的结果
        while (true) {
            int fetch_result = mysql_stmt_fetch(stmt);
            if (fetch_result == MYSQL_NO_DATA) {
                break; // 没有更多数据
            } else if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
                throw DatabaseException("Fetch failed with error code: " + std::to_string(fetch_result));
            }
            
            // 处理当前行的数据
            for (int i = 0; i < columnCount; i++) {
                if (isNull[i]) {
                    rowData[i] = ""; // NULL值处理为空字符串
                    rowLengths[i] = 0;
                } else {
                    // 处理可能的数据截断
                    if (lengths[i] > buffers[i].size()) {
                        // 重新分配更大的缓冲区
                        buffers[i].resize(lengths[i]);
                        binds[i].buffer = buffers[i].data();
                        binds[i].buffer_length = lengths[i];
                        
                        // 重新获取字段数据
                        mysql_stmt_fetch_column(stmt, &binds[i], i, 0);
                    }
                    
                    rowData[i] = static_cast<char*>(binds[i].buffer);
                    rowLengths[i] = lengths[i];
                }
            }
            
            // 使用子类实现的方法创建实体并添加到结果集
            results.push_back(createEntityFromRow(const_cast<char**>(rowData.data()), rowLengths.data()));
        }
        
        return results;
    }
    
    // 处理结果集
    std::vector<T> processResultSet(MYSQL_RES* res) const {
        std::vector<T> results;
        int numFields = mysql_num_fields(res);
        MYSQL_ROW row;
        unsigned long* lengths;
        
        while ((row = mysql_fetch_row(res))) {
            lengths = mysql_fetch_lengths(res);
            if (!lengths) {
                mysql_free_result(res);
                throw DatabaseException("Lengths fetch error");
            }
            
            T entity = createEntityFromRow(row, lengths);
            // 直接添加到结果集，不需要更新字段映射
            results.push_back(std::move(entity));
        }
        
        mysql_free_result(res);
        return results;
    }
};

#endif // !FK_BASE_MAPPER_HPP_