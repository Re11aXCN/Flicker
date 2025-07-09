#include "FKUserService.h"
#include <print>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "MySQL/Entity/FKUserEntity.h"

SINGLETON_CREATE_CPP(FKUserService)

FKUserService::FKUserService() {
    std::println("FKUserService 已创建");
    _initialize();
}

FKUserService::~FKUserService() {
    std::println("FKUserService 已销毁");
}

bool FKUserService::_initialize() {
    try {
        return _pUserMapper.createTableIfNotExists();
    } catch (const std::exception& e) {
        std::println("初始化用户管理器失败: {}", e.what());
        return false;
    }
}

DbOperator::UserRegisterResult FKUserService::registerUser(const std::string& username, const std::string& email, 
                                             const std::string& password, const std::string& salt) {
    if (username.empty() || email.empty() || password.empty()) {
        std::println("注册用户失败: 参数无效");
        return DbOperator::UserRegisterResult::INVALID_PARAMETERS;
    }

    try {
        if (_pUserMapper.findUserByUsername(username)) {
            std::println("注册用户失败: 用户名已存在 {}", username);
            return DbOperator::UserRegisterResult::USERNAME_EXISTS;
        }
        if (_pUserMapper.findUserByEmail(email)) {
            std::println("注册用户失败: 邮箱已存在 {}", email);
            return DbOperator::UserRegisterResult::EMAIL_EXISTS;
        }

        FKUserEntity user{ 
            0,
            boost::uuids::to_string(boost::uuids::random_generator()()),
            username,
            email,
            password,
            salt
        };

        // 插入用户
        if (!_pUserMapper.insertUser(user)) {
            std::println("注册用户失败: 数据库错误");
            return DbOperator::UserRegisterResult::DATABASE_ERROR;
        }

        std::println("用户注册成功: {}", user.toString());
        return DbOperator::UserRegisterResult::SUCCESS;
    } catch (const std::exception& e) {
        std::println("注册用户异常: {}", e.what());
        return DbOperator::UserRegisterResult::DATABASE_ERROR;
    }
}

bool FKUserService::isUsernameExists(const std::string& username) {
    try {
        return _pUserMapper.findUserByUsername(username).has_value();
    } catch (const std::exception& e) {
        std::println("检查用户名是否存在异常: {}", e.what());
        return false;
    }
}

bool FKUserService::isEmailExists(const std::string& email) {
    try {
        return _pUserMapper.findUserByEmail(email).has_value();
    } catch (const std::exception& e) {
        std::println("检查邮箱是否存在异常: {}", e.what());
        return false;
    }
}

std::optional<FKUserEntity> FKUserService::findUserById(int id) {
    try {
        return _pUserMapper.findUserById(id);
    } catch (const std::exception& e) {
        std::println("根据ID查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByUuid(const std::string& uuid) {
    try {
        return _pUserMapper.findUserByUuid(uuid);
    } catch (const std::exception& e) {
        std::println("根据UUID查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByUsername(const std::string& username) {
    try {
        return _pUserMapper.findUserByUsername(username);
    } catch (const std::exception& e) {
        std::println("根据用户名查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByEmail(const std::string& email) {
    try {
        return _pUserMapper.findUserByEmail(email);
    } catch (const std::exception& e) {
        std::println("根据邮箱查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::vector<FKUserEntity> FKUserService::getAllUsers() {
    try {
        return _pUserMapper.findAllUsers();
    } catch (const std::exception& e) {
        std::println("获取所有用户异常: {}", e.what());
        return {};
    }
}