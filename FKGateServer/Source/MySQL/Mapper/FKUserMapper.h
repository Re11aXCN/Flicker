/*************************************************************************************
 *
 * @ Filename	 : FKUserMapper.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
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
#include <chrono>
#include <mysql.h>

class FKUserEntity;
class FKMySQLConnectionPool;
class FKUserMapper {
public:
    // 构造函数
    explicit FKUserMapper(const std::string& tableName = "users");
    ~FKUserMapper();
    // 创建用户表（如果不存在）
    bool createTableIfNotExists();

    // 插入用户
    bool insertUser(FKUserEntity& user);

    // 根据ID查询用户
    std::optional<FKUserEntity> findUserById(int id);

    // 根据UUID查询用户
    std::optional<FKUserEntity> findUserByUuid(const std::string& uuid);

    // 根据用户名查询用户
    std::optional<FKUserEntity> findUserByUsername(const std::string& username);

    // 根据邮箱查询用户
    std::optional<FKUserEntity> findUserByEmail(const std::string& email);

    // 更新用户信息
    bool updateUser(const FKUserEntity& user);

    // 删除用户
    bool deleteUser(int id);

    // 查询所有用户
    std::vector<FKUserEntity> findAllUsers();

private:
    // 通用查询方法，根据条件查询单个用户
    std::optional<FKUserEntity> _findUserByCondition(const std::string& whereClause, 
                                                    const std::string& paramValue);

    // 从结果集构建用户实体
    FKUserEntity _buildUserFromRow(MYSQL_RES* result, MYSQL_ROW row);

    // 解析MySQL时间戳字符串为std::chrono::system_clock::time_point
    std::chrono::system_clock::time_point parseTimestamp(const std::string& timestampStr);

    // 表名
    std::string _tableName;
    
    // 连接池
    FKMySQLConnectionPool* _pConnectionPool;
};

#endif // !FK_USER_MAPPER_H_