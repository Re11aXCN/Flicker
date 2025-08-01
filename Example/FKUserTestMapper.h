﻿/*************************************************************************************
 *
 * @ Filename     : UserTestMapper.h
 * @ Description : 测试实体数据库映射器
 * 
 * @ Version     : V1.0
 * @ Author      : Re11a
 * @ Date Created: 2025/7/28
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef TEST_MAPPER_H_
#define TEST_MAPPER_H_

#include "Common/mysql/mapper/base_mapper.hpp"
#include "UserTestEntity.h"

class UserTestMapper : public BaseMapper<UserTestEntity, std::uint32_t> {
public:
    UserTestMapper();
    ~UserTestMapper() override = default;

    // 基本查询方法
    std::optional<UserTestEntity> findByEmail(const std::string& email);
    std::optional<UserTestEntity> findByUsername(const std::string& username);
    
    // 测试各种查询条件的方法
    void testTrueCondition();
    void testEqualCondition();
    void testNotEqualCondition();
    void testBetweenCondition();
    void testGreaterThanCondition();
    void testLessThanCondition();
    void testLikeCondition();
    void testRegexpCondition();
    void testInCondition();
    void testNotInCondition();
    void testIsNullCondition();
    void testAndCondition();
    void testOrCondition();
    void testNotCondition();
    void testRawCondition();
    
    // 测试更新和删除操作
    void testUpdateByCondition();
    void testDeleteByCondition();
    
    // 测试复杂查询
    void testComplexCondition();
    
protected:
    // 实现基类的虚函数
    constexpr std::string getTableName() const override;
    constexpr std::string findByIdQuery() const override;
    constexpr std::string findAllQuery() const override;
    constexpr std::string insertQuery() const override;
    constexpr std::string deleteByIdQuery() const override;
    
    void bindInsertParams(MySQLStmtPtr& stmtPtr, const UserTestEntity& entity) const override;
    
    UserTestEntity createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
        char* isNulls, size_t columnCount) const override;
    UserTestEntity createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields,
        unsigned long* lengths, size_t columnCount) const override;
    
    // 自定义查询方法
    constexpr std::string findByEmailQuery() const;
    constexpr std::string findByUsernameQuery() const;
};

#endif // !TEST_MAPPER_H_