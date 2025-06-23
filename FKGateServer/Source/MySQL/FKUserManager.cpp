#include "FKUserManager.h"
#include "Mapper/FKUserMapper.h"
#include <print>

SINGLETON_CREATE_CPP(FKUserManager)

FKUserManager::FKUserManager() {
    std::println("FKUserManager 已创建");
    _pConnectionPool = FKMySQLConnectionPool::getInstance();
    _initialize();
}

FKUserManager::~FKUserManager() {
    std::println("FKUserManager 已销毁");
}

bool FKUserManager::_initialize() {
    try {
        // 创建用户表
        return _pConnectionPool->executeWithConnection([](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.createTableIfNotExists();
        });
    } catch (const std::exception& e) {
        std::println("初始化用户管理器失败: {}", e.what());
        return false;
    }
}

UserRegisterResult FKUserManager::registerUser(const std::string& username, const std::string& email, 
                                             const std::string& password) {
    // 参数验证
    if (username.empty() || email.empty() || password.empty()) {
        std::println("注册用户失败: 参数无效");
        return UserRegisterResult::INVALID_PARAMETERS;
    }

    try {
        // 使用事务执行注册操作
        return _pConnectionPool->executeTransaction([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);

            // 检查用户名是否存在
            if (mapper.findUserByUsername(username)) {
                std::println("注册用户失败: 用户名已存在 {}", username);
                return UserRegisterResult::USERNAME_EXISTS;
            }

            // 检查邮箱是否存在
            if (mapper.findUserByEmail(email)) {
                std::println("注册用户失败: 邮箱已存在 {}", email);
                return UserRegisterResult::EMAIL_EXISTS;
            }

            // 创建用户实体
            FKUserEntity user(username, email, password);

            // 插入用户
            if (!mapper.insertUser(user)) {
                std::println("注册用户失败: 数据库错误");
                return UserRegisterResult::DATABASE_ERROR;
            }

            std::println("用户注册成功: {}", user.toString());
            return UserRegisterResult::SUCCESS;
        });
    } catch (const std::exception& e) {
        std::println("注册用户异常: {}", e.what());
        return UserRegisterResult::DATABASE_ERROR;
    }
}

bool FKUserManager::isUsernameExists(const std::string& username) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserByUsername(username).has_value();
        });
    } catch (const std::exception& e) {
        std::println("检查用户名是否存在异常: {}", e.what());
        return false;
    }
}

bool FKUserManager::isEmailExists(const std::string& email) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserByEmail(email).has_value();
        });
    } catch (const std::exception& e) {
        std::println("检查邮箱是否存在异常: {}", e.what());
        return false;
    }
}

std::optional<FKUserEntity> FKUserManager::findUserById(int id) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserById(id);
        });
    } catch (const std::exception& e) {
        std::println("根据ID查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserManager::findUserByUuid(const std::string& uuid) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserByUuid(uuid);
        });
    } catch (const std::exception& e) {
        std::println("根据UUID查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserManager::findUserByUsername(const std::string& username) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserByUsername(username);
        });
    } catch (const std::exception& e) {
        std::println("根据用户名查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserManager::findUserByEmail(const std::string& email) {
    try {
        return _pConnectionPool->executeWithConnection([&](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findUserByEmail(email);
        });
    } catch (const std::exception& e) {
        std::println("根据邮箱查询用户异常: {}", e.what());
        return std::nullopt;
    }
}

std::vector<FKUserEntity> FKUserManager::getAllUsers() {
    try {
        return _pConnectionPool->executeWithConnection([](mysqlx::Session* session) {
            FKUserMapper mapper(session);
            return mapper.findAllUsers();
        });
    } catch (const std::exception& e) {
        std::println("获取所有用户异常: {}", e.what());
        return {};
    }
}