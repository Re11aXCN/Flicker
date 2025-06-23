/*************************************************************************************
 *
 * @ Filename	 : FKUserMapper.h
 * @ Description : 用户数据访问层，提供用户相关的数据库操作
 *
 * @ Version	 : V1.0
 * @ Author	 : Re11a
 * @ Date Created: 2025/7/15
*************************************************************************************/
#ifndef FK_USER_MAPPER_H_
#define FK_USER_MAPPER_H_

#include <string>
#include <vector>
#include <optional>
#include <mysqlx/xdevapi.h>
#include "../Entity/FKUserEntity.h"

// 用户数据访问层
class FKUserMapper {
public:
    // 构造函数
    explicit FKUserMapper(mysqlx::Session* session) : _session(session) {}

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
    // 从结果集构建用户实体
    FKUserEntity _buildUserFromRow(const mysqlx::Row& row);

    // 数据库会话
    mysqlx::Session* _session;

    // 表名
    const std::string _tableName = "users";
};

#endif // !FK_USER_MAPPER_H_