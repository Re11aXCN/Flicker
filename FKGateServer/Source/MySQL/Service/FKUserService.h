/*************************************************************************************
 *
 * @ Filename	 : FKUserService.h
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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "FKDef.h"
#include "FKMacro.h"
#include "MySQL/Mapper/FKUserMapper.h"
class FKUserEntity;
// 用户管理类，使用单例模式
class FKUserService {
    SINGLETON_CREATE_H(FKUserService)

public:
    DbOperator::UserRegisterResult registerUser(const std::string& username, const std::string& email,
                                   const std::string& password, const std::string& salt);
    bool isUsernameExists(const std::string& username);
    bool isEmailExists(const std::string& email);

    std::optional<FKUserEntity> findUserById(int id);
    std::optional<FKUserEntity> findUserByUuid(const std::string& uuid);
    std::optional<FKUserEntity> findUserByUsername(const std::string& username);
    std::optional<FKUserEntity> findUserByEmail(const std::string& email);
    std::vector<FKUserEntity> getAllUsers();

private:
    FKUserService();
    ~FKUserService();
	// 初始化，创建用户表
	bool _initialize();

    // 用户数据访问对象
    FKUserMapper _pUserMapper;
};

#endif // !FK_USER_MANAGER_H_