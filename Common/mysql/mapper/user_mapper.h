/*************************************************************************************
 *
 * @ Filename     : UserMapper.h
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
#ifndef USER_MAPPER_H_
#define USER_MAPPER_H_

#include "Common/mysql/mapper/base_mapper.hpp"
#include "Common/mysql/entity/user_entity.h"
class UserMapper final : public BaseMapper<UserEntity, std::uint32_t> {
public:
    UserMapper();
    ~UserMapper() override = default;

    std::optional<UserEntity> findByEmail(const std::string& email);
    std::optional<UserEntity> findByUsername(const std::string& username);
    DbOperator::Status updatePasswordByEmail(const std::string& email, const std::string& password);
    DbOperator::Status deleteByEmail(const std::string& email);

    bool isUsernameExists(const std::string& username);
    bool isEmailExists(const std::string& email);
    std::optional<std::string> findUuidByEmail(const std::string& email);
    std::optional<std::string> findUuidByUsername(const std::string& username);
    std::optional<std::string> findPasswordByEmail(const std::string& email);
    std::optional<std::string> findPasswordByUsername(const std::string& username);
    
    // 使用查询条件策略的新方法
    //std::vector<UserEntity> findByCondition(const std::shared_ptr<IQueryCondition>& condition);
    //std::optional<UserEntity> findOneByCondition(const std::shared_ptr<IQueryCondition>& condition);
    //bool existsByCondition(const std::shared_ptr<IQueryCondition>& condition);
    //DbOperator::Status updatePasswordByCondition(const std::shared_ptr<IQueryCondition>& condition, const std::string& password);
    
protected:
    // 实现基类的虚函数
    constexpr std::string getTableName() const override;
    constexpr std::string findByIdQuery() const override;
    constexpr std::string findAllQuery() const override;
    constexpr std::string insertQuery() const override;
    constexpr std::string deleteByIdQuery() const override;
    
    void bindInsertParams(MySQLStmtPtr& stmtPtr, const UserEntity& entity) const override;
    
    UserEntity createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
        char* isNulls, size_t columnCount) const override;
    UserEntity createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields,
        unsigned long* lengths, size_t columnCount) const override;

    // 自定义查询方法
    constexpr std::string findByEmailQuery() const;
    constexpr std::string findByUsernameQuery() const;
    constexpr std::string updatePasswordByEmailQuery() const;
    constexpr std::string deleteByEmailQuery() const;
    
private:
    constexpr std::string _isUsernameExistsQuery() const;
    constexpr std::string _isEmailExistsQuery() const;
    constexpr std::string _findPasswordByEmailQuery();
    constexpr std::string _findPasswordByUsernameQuery();
};

#endif // !USER_MAPPER_H_