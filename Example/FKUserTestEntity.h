/*************************************************************************************
 *
 * @ Filename     : UserTestEntity.h
 * @ Description : 测试实体类，用于测试查询条件功能
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
#ifndef USER_TEST_ENTITY_H_
#define USER_TEST_ENTITY_H_

#include "Common/utils/utils.h"
#include "Common/mysql/mapper/field_mapper.hpp"
#include "Common/mysql/entity/base_entity.h"
class UserTestMapper;
class UserTestEntity : public BaseEntity {
    friend class UserTestMapper;
public:
    using FieldTypeList = TypeList<
        std::uint32_t,                                       // id
        std::string,                                         // uuid
        std::string,                                         // username
        std::string,                                         // email
        std::string,                                         // password
        std::chrono::system_clock::time_point,               // create_time
        std::optional<std::chrono::system_clock::time_point> // update_time
    >;
    static constexpr std::array<const char*, 7> FIELD_NAMES = {
       "id", "uuid", "username", "email", "password", "create_time", "update_time"
    };

    explicit UserTestEntity() = default;
    ~UserTestEntity() = default;

    UserTestEntity(const std::string& username,
                const std::string& email, const std::string& password)
        : _username(username), _email(email), _password(password)
    {
        _username_length = static_cast<unsigned long>(_username.length());
        _email_length = static_cast<unsigned long>(_email.length());
        _password_length = static_cast<unsigned long>(_password.length());
    }

    UserTestEntity(std::uint32_t id, const std::string& uuid, const std::string& username,
        const std::string& email, const std::string& password,
        const std::chrono::system_clock::time_point& createTime, const std::optional<std::chrono::system_clock::time_point>& updateTime)
        : _id(id), _uuid(uuid), _username(username), _email(email), _password(password), _createTime(createTime), _updateTime(updateTime) 
    {
        _username_length = static_cast<unsigned long>(_username.length());
        _email_length = static_cast<unsigned long>(_email.length());
        _password_length = static_cast<unsigned long>(_password.length());
    }

    UserTestEntity(std::uint32_t id, std::string&& uuid, std::string&& username,
        std::string&& email, std::string&& password,
        std::chrono::system_clock::time_point&& createTime, std::optional<std::chrono::system_clock::time_point>&& updateTime)
        : _id(id), _uuid(std::move(uuid)), _username(std::move(username)), _email(std::move(email)), _password(std::move(password)), _createTime(std::move(createTime)), _updateTime(std::move(updateTime))
    {
        _username_length = static_cast<unsigned long>(_username.length());
        _email_length = static_cast<unsigned long>(_email.length());
        _password_length = static_cast<unsigned long>(_password.length());
    }

    UserTestEntity(const UserTestEntity& other) = default;
    UserTestEntity& operator=(const UserTestEntity& other) = default;

    UserTestEntity(UserTestEntity&& other) = default;
    UserTestEntity& operator=(UserTestEntity&& other) = default;

    // Getters
    const std::uint32_t& getId() const { return _id; }
    const std::string& getUuid() const { return _uuid; }
    const std::string& getUsername() const { return _username; }
    const std::string& getEmail() const { return _email; }
    const std::string& getPassword() const { return _password; }
    const std::chrono::system_clock::time_point& getCreateTime() const { return _createTime; }
    const std::optional<std::chrono::system_clock::time_point>& getUpdateTime() const { return _updateTime; }

    const unsigned long* getUsernameLength() const { return &_username_length; }
    const unsigned long* getEmailLength() const { return &_email_length; }
    const unsigned long* getPasswordLength() const { return &_password_length; }

    // Setters
    void setId(std::uint32_t id) { _id = id; }
    void setUuid(const std::string& uuid) { _uuid = uuid; }
    void setUsername(const std::string& username) { _username = username; }
    void setEmail(const std::string& email) { _email = email; }
    void setPassword(const std::string& password) { _password = password; }
    void setCreateTime(const std::chrono::system_clock::time_point& createTime) { _createTime = createTime; }
    void setUpdateTime(const std::optional<std::chrono::system_clock::time_point>& updateTime) { _updateTime = updateTime; }

    // 转换为字符串表示
    std::vector<std::string> getFieldNames() const override {
        return { FIELD_NAMES.begin(), FIELD_NAMES.end() };
    }

    std::vector<std::string> getFieldValues() const override {
        return { std::to_string(_id), _uuid, _username, _email, _password,
            utils::time_point_to_str(_createTime),
            _updateTime ? utils::time_point_to_str(*_updateTime) : "NULL"
        };
    }

private:
    std::uint32_t _id{0};
    std::string _uuid{};
    std::string _username;
    std::string _email;
    std::string _password;
    std::chrono::system_clock::time_point _createTime{};
    std::optional<std::chrono::system_clock::time_point> _updateTime{std::nullopt};

    unsigned long _username_length{0};
    unsigned long _email_length{0};
    unsigned long _password_length{0};
};


#endif // !USER_TEST_ENTITY_H_