#include "FKUserEntity.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <print>
/*
// 生成UUID
std::string FKUserEntity::generateUuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;

    // 生成UUID格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    // 其中y是8,9,a,b中的一个
    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";

    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";

    ss << dis2(gen);

    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }

    return ss.str();
}
*/
// 转换为字符串表示
std::string FKUserEntity::toString() const {
    std::ostringstream oss;
    oss << "User {";
    oss << "id=" << _id << ", ";
    oss << "uuid='" << _uuid << "', ";
    oss << "username='" << _username << "', ";
    oss << "email='" << _email << "', ";
    oss << "password='******', "; // 不显示实际密码
    
    // 格式化时间
    auto timeToString = [](const auto& timePoint) {
        auto time = std::chrono::system_clock::to_time_t(timePoint);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    };
    
    oss << "createTime='" << timeToString(_createTime) << "'";
    
    if (_updateTime) {
        oss << ", updateTime='" << timeToString(*_updateTime) << "'";
    }
    
    oss << "}";
    return oss.str();
}