#include "FKUserService.h"

#include "FKLogger.h"
#include "MySQL/Entity/FKUserEntity.h"

FKUserService::FKUserService() {
    _initialize();
}

FKUserService::~FKUserService() {
}

bool FKUserService::_initialize() {
    try {
        return _pUserMapper.createTableIfNotExists();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("初始化FKUserService失败: {}", e.what()));
        return false;
    }
}

DbOperator::Status FKUserService::registerUser(const std::string& username, const std::string& email, const std::string& password)
{
    if (username.empty() || email.empty() || password.empty()) {
        FK_SERVER_ERROR("注册用户失败: 参数无效");
        return DbOperator::Status::InvalidParameters;
    }

    try {
        if (_pUserMapper.isUsernameExists(username)) {
            FK_SERVER_ERROR(std::format("注册用户失败: 用户名已存在 {}", username));
            return DbOperator::Status::DataAlreadyExist;
        }
        if (_pUserMapper.isEmailExists(email)) {
            FK_SERVER_ERROR(std::format("注册用户失败: 邮箱已存在 {}", email));
            return DbOperator::Status::DataAlreadyExist;
        }

        FKUserEntity user{username, email, password};

        // 插入用户
        if (!_pUserMapper.insertUser(user)) {
            return DbOperator::Status::DatabaseError;
        }

        FK_SERVER_INFO(std::format("用户注册成功: {}", user.toString()));
        return DbOperator::Status::Success;
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("注册用户异常: {}", e.what()));
        return DbOperator::Status::DatabaseError;
    }
}

bool FKUserService::isUsernameExists(const std::string& username) {
    try {
        return _pUserMapper.findUserByUsername(username).has_value();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("检查用户名是否存在异常: {}", e.what()));
        return false;
    }
}

bool FKUserService::isEmailExists(const std::string& email) {
    try {
        return _pUserMapper.findUserByEmail(email).has_value();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("检查邮箱是否存在异常: {}", e.what()));
        return false;
    }
}

std::optional<FKUserEntity> FKUserService::findUserById(int id) {
    try {
        return _pUserMapper.findUserById(id);
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("根据ID查询用户异常: {}", e.what()));
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByUuid(const std::string& uuid) {
    try {
        return _pUserMapper.findUserByUuid(uuid);
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("根据UUID查询用户异常: {}", e.what()));
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByUsername(const std::string& username) {
    try {
        return _pUserMapper.findUserByUsername(username);
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("根据用户名查询用户异常: {}", e.what()));
        return std::nullopt;
    }
}

std::optional<FKUserEntity> FKUserService::findUserByEmail(const std::string& email) {
    try {
        return _pUserMapper.findUserByEmail(email);
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("根据邮箱查询用户异常: {}", e.what()));
        return std::nullopt;
    }
}

std::vector<FKUserEntity> FKUserService::getAllUsers() {
    try {
        return _pUserMapper.findAllUsers();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("获取所有用户异常: {}", e.what()));
        return {};
    }
}