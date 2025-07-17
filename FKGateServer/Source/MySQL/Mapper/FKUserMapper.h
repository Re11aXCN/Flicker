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
    
    // 根据邮箱查找用户
    std::optional<FKUserEntity> findByEmail(const std::string& email);
    
    // 根据用户名查找用户
    std::optional<FKUserEntity> findByUsername(const std::string& username);
    
    // 根据邮箱更新密码和更新时间
    DbOperator::Status updatePasswordByEmail(const std::string& email, 
                                           const std::string& password);
    
    // 根据邮箱删除用户
    DbOperator::Status deleteByEmail(const std::string& email);
    
protected:
    // 实现基类的虚函数
    std::string getTableName() const override;
    std::string findByIdQuery() const override;
    std::string findAllQuery() const override;
    std::string insertQuery() const override;
    std::string deleteByIdQuery() const override;
    
    void bindIdParam(MYSQL_STMT* stmt, std::size_t id) const override;
    void bindInsertParams(MYSQL_STMT* stmt, const FKUserEntity& entity) const override;
    
    FKUserEntity createEntityFromRow(MYSQL_ROW row, unsigned long* lengths) const override;
    
    // 自定义查询方法
    std::string findByEmailQuery() const;
    std::string findByUsernameQuery() const;
    std::string updatePasswordByEmailQuery() const;
    std::string deleteByEmailQuery() const;
    
    // 自定义参数绑定方法
    void bindEmailParam(MYSQL_STMT* stmt, const std::string& email) const;
    void bindUsernameParam(MYSQL_STMT* stmt, const std::string& username) const;
    void bindPasswordAndEmailParams(MYSQL_STMT* stmt, const std::string& password, 
                                   const std::string& email) const;
};

#endif // !FK_USER_MAPPER_H_