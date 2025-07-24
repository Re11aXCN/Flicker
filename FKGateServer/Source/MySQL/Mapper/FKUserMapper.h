/*************************************************************************************
 *
 * @ Filename     : FKUserMapper.h
 * @ Description : 用户实体数据库映射器
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
#ifndef FK_USER_MAPPER_H_
#define FK_USER_MAPPER_H_

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mysql.h>

#include "../Entity/FKUserEntity.h"
#include "FKBaseMapper.hpp"
class FKUserMapper : public FKBaseMapper<FKUserEntity, std::size_t> {
public:
    FKUserMapper();
    ~FKUserMapper() override = default;

    std::optional<FKUserEntity> findByEmail(const std::string& email);
    std::optional<FKUserEntity> findByUsername(const std::string& username);
    DbOperator::Status updatePasswordByEmail(const std::string& email, const std::string& password);
    DbOperator::Status deleteByEmail(const std::string& email);

    bool isUsernameExists(const std::string& username);
    bool isEmailExists(const std::string& email);
    std::optional<std::string> findPasswordByEmail(const std::string& email);
    std::optional<std::string> findPasswordByUsername(const std::string& username);
    
protected:
    // 实现基类的虚函数
    constexpr std::string getTableName() const override;
    constexpr std::string findByIdQuery() const override;
    constexpr std::string findAllQuery() const override;
    constexpr std::string insertQuery() const override;
    constexpr std::string deleteByIdQuery() const override;
    
    void bindIdParam(MySQLStmtPtr& stmtPtr, std::size_t* id) const override;
    void bindInsertParams(MySQLStmtPtr& stmtPtr, const FKUserEntity& entity) const override;
    
    FKUserEntity createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const override;
    
    // 自定义查询方法
    constexpr std::string findByEmailQuery() const;
    constexpr std::string findByUsernameQuery() const;
    constexpr std::string updatePasswordByEmailQuery() const;
    constexpr std::string deleteByEmailQuery() const;
    
    // 自定义参数绑定方法
    void bindEmailParam(MySQLStmtPtr& stmtPtr, const std::string& email, unsigned long* length) const;
    void bindUsernameParam(MySQLStmtPtr& stmtPtr, const std::string& username, unsigned long* length) const;
    void bindPasswordAndEmailParams(MySQLStmtPtr& stmtPtr, const std::string& password, unsigned long* passwordLength,
                                   const std::string& email, unsigned long* emailLength) const;

private:
    constexpr std::string _isUsernameExistsQuery() const;
    constexpr std::string _isEmailExistsQuery() const;
    constexpr std::string _findPasswordByEmailQuery();
    constexpr std::string _findPasswordByUsernameQuery();
};

#endif // !FK_USER_MAPPER_H_