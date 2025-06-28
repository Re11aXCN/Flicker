/*************************************************************************************
 *
 * @ Filename	 : FKUserManager.h
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
#ifndef FK_USER_MANAGER_H_
#define FK_USER_MANAGER_H_

#include <string>
#include <memory>
#include <optional>
#include "FKDef.h"
#include "FKMacro.h"
#include "Entity/FKUserEntity.h"
#include "FKMySQLConnectionPool.h"

// 用户管理类，使用单例模式
class FKUserManager {
    SINGLETON_CREATE_H(FKUserManager)

public:

    // 注册用户
    DbOperator::UserRegisterResult registerUser(const std::string& username, const std::string& email,
                                   const std::string& password);

    // 检查用户名是否存在
    bool isUsernameExists(const std::string& username);

    // 检查邮箱是否存在
    bool isEmailExists(const std::string& email);

    // 根据ID查询用户
    std::optional<FKUserEntity> findUserById(int id);

    // 根据UUID查询用户
    std::optional<FKUserEntity> findUserByUuid(const std::string& uuid);

    // 根据用户名查询用户
    std::optional<FKUserEntity> findUserByUsername(const std::string& username);

    // 根据邮箱查询用户
    std::optional<FKUserEntity> findUserByEmail(const std::string& email);

    // 获取所有用户
    std::vector<FKUserEntity> getAllUsers();

private:
    FKUserManager();
    ~FKUserManager();
	// 初始化，创建用户表
	bool _initialize();

    // MySQL连接池
    FKMySQLConnectionPool* _pConnectionPool;
};

#endif // !FK_USER_MANAGER_H_