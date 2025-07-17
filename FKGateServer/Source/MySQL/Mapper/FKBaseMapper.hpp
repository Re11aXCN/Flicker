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
#include <mysql.h>

#include "../FKMySQLConnectionPool.h"
#include "FKDef.h"
#include "FKLogger.h"
#include "FKFieldMapper.hpp"
// 数据库异常类
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message) : std::runtime_error(message) {}
};

// 基础数据库映射器模板类
template<typename T, typename ID_TYPE>
class FKBaseMapper {
public:
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
            MYSQL_STMT* stmt = prepareStatement(findByIdQuery().c_str());
            if (!stmt) {
                throw DatabaseException("Failed to prepare statement for findById");
            }
            
            // 绑定参数
            bindIdParam(stmt, id);
            
            // 执行查询
            executeQuery(stmt);
            
            // 获取元数据
            MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
            if (!meta) {
                mysql_stmt_close(stmt);
                throw DatabaseException("No metadata available for findById query");
            }
            
            // 获取结果
            std::optional<T> result;
            try {
                result = fetchSingleResult(stmt, meta);
            } catch (const DatabaseException& e) {
                // 如果没有找到结果，返回空optional
                if (std::string(e.what()).find("No results found") != std::string::npos) {
                    result = std::nullopt;
                } else {
                    throw; // 重新抛出其他异常
                }
            }
            
            // 清理资源
            mysql_free_result(meta);
            mysql_stmt_close(stmt);
            
            return result;
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
            std::string query = findAllQuery() + " LIMIT " + std::to_string(limit) + 
                               " OFFSET " + std::to_string(offset);
            
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
            MYSQL_STMT* stmt = prepareStatement(insertQuery().c_str());
            if (!stmt) {
                throw DatabaseException("Failed to prepare statement for insert");
            }
            
            // 绑定参数
            bindInsertParams(stmt, entity);
            
            // 执行更新
            executeUpdate(stmt);
            
            // 清理资源
            mysql_stmt_close(stmt);
            
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
            MYSQL_STMT* stmt = prepareStatement(deleteByIdQuery().c_str());
            if (!stmt) {
                throw DatabaseException("Failed to prepare statement for deleteById");
            }
            
            // 绑定参数
            bindIdParam(stmt, id);
            
            // 执行更新
            executeUpdate(stmt);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmt);
            
            // 清理资源
            mysql_stmt_close(stmt);
            
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("deleteById error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
        }
    }
    
    // 根据ID更新指定字段
    template<typename... Args>
    DbOperator::Status updateFieldsById(ID_TYPE id, const std::string& fields, Args&&... args) {
        try {
            std::string query = "UPDATE " + getTableName() + " SET " + fields + 
                               " WHERE id = ?";
            
            MYSQL_STMT* stmt = prepareStatement(query.c_str());
            if (!stmt) {
                throw DatabaseException("Failed to prepare statement for updateFieldsById");
            }
            
            // 绑定参数
            bindUpdateParams(stmt, id, std::forward<Args>(args)...);
            
            // 执行更新
            executeUpdate(stmt);
            
            // 检查影响的行数
            my_ulonglong affectedRows = mysql_stmt_affected_rows(stmt);
            
            // 清理资源
            mysql_stmt_close(stmt);
            
            if (affectedRows == 0) {
                return DbOperator::Status::DataNotExist;
            }
            
            return DbOperator::Status::Success;
        } catch (const std::exception& e) {
            FK_SERVER_ERROR(std::format("updateFieldsById error: {}", e.what()));
            return DbOperator::Status::DatabaseError;
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
    virtual std::string getTableName() const = 0;
    virtual std::string findByIdQuery() const = 0;
    virtual std::string findAllQuery() const = 0;
    virtual std::string insertQuery() const = 0;
    virtual std::string deleteByIdQuery() const = 0;
    
    // 子类必须实现的参数绑定方法
    virtual void bindIdParam(MYSQL_STMT* stmt, ID_TYPE id) const = 0;
    virtual void bindInsertParams(MYSQL_STMT* stmt, const T& entity) const = 0;
    
    // 子类必须实现的结果处理方法
    virtual T createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const = 0;
    
    // 更新参数绑定方法（变参模板）
    template<typename... Args>
    void bindUpdateParams(MYSQL_STMT* stmt, ID_TYPE id, Args&&... args) const {
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
        std::vector<char> buffer(strValue.begin(), strValue.end());
        buffer.push_back('\0'); // 确保以null结尾
        
        unsigned long length = static_cast<unsigned long>(strValue.length());
        
        binds[index].buffer_type = MYSQL_TYPE_STRING;
        binds[index].buffer = buffer.data();
        binds[index].buffer_length = length;
        binds[index].length = &length;
        binds[index].is_null = 0;
        
        // 递归处理下一个参数
        bindUpdateParamsImpl(binds, index + 1, std::forward<Rest>(rest)...);
    }

    // 准备预处理语句
    MYSQL_STMT* prepareStatement(const char* query) const {
        return FKMySQLConnectionPool::getInstance()->executeWithConnection(
            [query](MYSQL* mysql) -> MYSQL_STMT* {
                MYSQL_STMT* stmt = mysql_stmt_init(mysql);
                if (!stmt) {
                    throw std::runtime_error("Statement init failed");
                }
                
                if (mysql_stmt_prepare(stmt, query, strlen(query))) {
                    std::string error = mysql_error(mysql);
                    mysql_stmt_close(stmt);
                    throw std::runtime_error("Prepare failed: " + error);
                }
                
                return stmt;
            }
        );
    }
    
    // 执行查询
    void executeQuery(MYSQL_STMT* stmt) const {
        if (mysql_stmt_execute(stmt)) {
            std::string error = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw DatabaseException("Execute failed: " + error);
        }
    }
    
    // 执行更新
    void executeUpdate(MYSQL_STMT* stmt) const {
        executeQuery(stmt); // 执行逻辑相同
    }
    
    // 获取单个结果
    T fetchSingleResult(MYSQL_STMT* stmt, MYSQL_RES* meta) const {
        if (mysql_stmt_store_result(stmt)) {
            std::string error = mysql_stmt_error(stmt);
            throw DatabaseException("Store result failed: " + error);
        }
        
        // 检查是否有结果
        if (mysql_stmt_num_rows(stmt) == 0) {
            throw DatabaseException("No results found");
        }
        
        // 获取字段数量
        int columnCount = mysql_num_fields(meta);
        
        // 使用智能指针管理内存，确保资源正确释放
        auto binds = std::make_unique<MYSQL_BIND[]>(columnCount);
        auto isNull = std::make_unique<char[]>(columnCount);
        auto lengths = std::make_unique<unsigned long[]>(columnCount);
        auto error = std::make_unique<char[]>(columnCount);
        
        // 初始化isNull数组
        std::fill_n(isNull.get(), columnCount, 0);
        std::fill_n(error.get(), columnCount, 0);
        
        // 分配内存用于存储结果 - 使用shared_ptr确保生命周期
        std::vector<std::shared_ptr<std::vector<char>>> buffers;
        buffers.reserve(columnCount);
        
        for (int i = 0; i < columnCount; i++) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(meta, i);
            size_t bufferSize = field->max_length > 0 ? field->max_length : 8192; // 增大默认缓冲区大小
            
            auto buffer = std::make_shared<std::vector<char>>(bufferSize);
            buffers.push_back(buffer);
            
            memset(&binds[i], 0, sizeof(MYSQL_BIND));
            binds[i].buffer_type = field->type;
            binds[i].buffer = buffer->data();
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
        
        // 获取结果
        int fetch_result = mysql_stmt_fetch(stmt);
        if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
            throw DatabaseException("Fetch failed with error code: " + std::to_string(fetch_result));
        }
        
        // 准备数据用于创建实体
        std::vector<std::string> rowData(columnCount);
        std::vector<unsigned long> rowLengths(columnCount);
        
        for (int i = 0; i < columnCount; i++) {
            if (isNull[i]) {
                rowData[i] = ""; // NULL值处理为空字符串
                rowLengths[i] = 0;
            } else {
                // 处理可能的数据截断
                if (lengths[i] > binds[i].buffer_length) {
                    // 重新分配更大的缓冲区
                    buffers[i]->resize(lengths[i]);
                    binds[i].buffer = buffers[i]->data();
                    binds[i].buffer_length = lengths[i];
                    
                    // 重新获取字段数据
                    mysql_stmt_fetch_column(stmt, &binds[i], i, 0);
                }
                
                rowData[i] = std::string(static_cast<char*>(binds[i].buffer), lengths[i]);
                rowLengths[i] = lengths[i];
            }
        }
        
        // 创建一个临时的MYSQL_ROW和lengths数组
        std::vector<const char*> rowPtrs(columnCount);
        for (int i = 0; i < columnCount; i++) {
            rowPtrs[i] = rowData[i].c_str();
        }
        
        // 使用子类实现的方法创建实体
        T entity = createEntityFromRow(const_cast<char**>(rowPtrs.data()), rowLengths.data());
        
        return entity;
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