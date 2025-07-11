﻿/*************************************************************************************
 *
 * @ Filename	 : FKUserEntity.h
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
#ifndef FK_USER_ENTITY_H_
#define FK_USER_ENTITY_H_

#include <string>
#include <chrono>
#include <optional>

// 用户实体类
class FKUserEntity {
public:
    // 构造函数
    explicit FKUserEntity() = default;
    ~FKUserEntity() = default;

    FKUserEntity(std::size_t id, const std::string& uuid, const std::string& username,
                const std::string& email, const std::string& password, const std::string& salt,
                const std::chrono::system_clock::time_point& createTime = std::chrono::system_clock::now(),
                const std::optional<std::chrono::system_clock::time_point>& updateTime = std::nullopt)
        : _id(id), _uuid(uuid), _username(username), _email(email), _password(password), _salt(salt),
          _createTime(createTime), _updateTime(updateTime) {}

    // Getters
    std::size_t getId() const { return _id; }
    const std::string& getUuid() const { return _uuid; }
    const std::string& getUsername() const { return _username; }
    const std::string& getEmail() const { return _email; }
    const std::string& getPassword() const { return _password; }
    const std::string& getSalt() const { return _salt; }
    const std::chrono::system_clock::time_point& getCreateTime() const { return _createTime; }
    const std::optional<std::chrono::system_clock::time_point>& getUpdateTime() const { return _updateTime; }

    // Setters
    void setId(std::size_t id) { _id = id; }
    void setUuid(const std::string& uuid) { _uuid = uuid; }
    void setUsername(const std::string& username) { _username = username; }
    void setEmail(const std::string& email) { _email = email; }
    void setPassword(const std::string& password) { _password = password; }
    void setSalt(const std::string& salt) { _salt = salt; }
    void setCreateTime(const std::chrono::system_clock::time_point& createTime) { _createTime = createTime; }
    void setUpdateTime(const std::optional<std::chrono::system_clock::time_point>& updateTime) { _updateTime = updateTime; }

    // 转换为字符串表示
    std::string toString() const;
private:
    std::size_t _id{0};
    std::string _uuid;
    std::string _username;
    std::string _email;
    std::string _password;
    std::string _salt;  // 存储BCrypt盐值
    std::chrono::system_clock::time_point _createTime;
    std::optional<std::chrono::system_clock::time_point> _updateTime;
};


#endif // !FK_USER_ENTITY_H_