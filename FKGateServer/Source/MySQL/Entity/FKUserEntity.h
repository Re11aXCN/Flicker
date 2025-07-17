/*************************************************************************************
 *
 * @ Filename     : FKUserEntity.h
 * @ Description : 
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
#ifndef FK_USER_ENTITY_H_
#define FK_USER_ENTITY_H_

#include <string>
#include <chrono>
#include <optional>
#include <array>
#include "../Mapper/FKFieldMapper.hpp"

class FKUserMapper;
class FKUserEntity {
    friend class FKUserMapper;
public:
    using FieldTypeList = TypeList<std::size_t, std::string, std::chrono::system_clock::time_point, std::optional<std::chrono::system_clock::time_point>>;
    static constexpr std::array<const char*, 7> FIELD_NAMES = {
       "id", "uuid", "username", "email", "password", "create_time", "update_time"
    };

    explicit FKUserEntity() = default;
    ~FKUserEntity() = default;

    FKUserEntity(const std::string& username,
                const std::string& email, const std::string& password)
        : _username(username), _email(email), _password(password) {}

    FKUserEntity(size_t id, const std::string& uuid, const std::string& username,
        const std::string& email, const std::string& password,
        const std::chrono::system_clock::time_point& createTime, const std::optional<std::chrono::system_clock::time_point>& updateTime)
        : _id(id), _uuid(uuid), _username(username), _email(email), _password(password), _createTime(createTime), _updateTime(updateTime) {
    }

    FKUserEntity(const FKUserEntity& other) = default;
    FKUserEntity& operator=(const FKUserEntity& other) = default;

    FKUserEntity(FKUserEntity&& other) = default;
    FKUserEntity& operator=(FKUserEntity&& other) = default;

    // Getters
    std::size_t getId() const { return _id; }
    const std::string& getUuid() const { return _uuid; }
    const std::string& getUsername() const { return _username; }
    const std::string& getEmail() const { return _email; }
    const std::string& getPassword() const { return _password; }
    const std::chrono::system_clock::time_point& getCreateTime() const { return _createTime; }
    const std::optional<std::chrono::system_clock::time_point>& getUpdateTime() const { return _updateTime; }

    // Setters
    void setId(std::size_t id) { _id = id; }
    void setUuid(const std::string& uuid) { _uuid = uuid; }
    void setUsername(const std::string& username) { _username = username; }
    void setEmail(const std::string& email) { _email = email; }
    void setPassword(const std::string& password) { _password = password; }
    void setCreateTime(const std::chrono::system_clock::time_point& createTime) { _createTime = createTime; }
    void setUpdateTime(const std::optional<std::chrono::system_clock::time_point>& updateTime) { _updateTime = updateTime; }

    // 转换为字符串表示
    std::string toString() const;
private:
    std::size_t _id{0};
    std::string _uuid{};
    std::string _username;
    std::string _email;
    std::string _password;
    std::chrono::system_clock::time_point _createTime{};
    std::optional<std::chrono::system_clock::time_point> _updateTime{std::nullopt};
};


#endif // !FK_USER_ENTITY_H_