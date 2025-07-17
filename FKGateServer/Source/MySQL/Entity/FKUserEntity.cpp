#include "FKUserEntity.h"
#include <sstream>

#include "FKUtils.h"

// 转换为字符串表示
std::string FKUserEntity::toString() const {
    std::ostringstream oss;
    oss << "User {";
    oss << "id=" << _id << ", ";
    oss << "uuid='" << _uuid << "', ";
    oss << "username='" << _username << "', ";
    oss << "email='" << _email << "', ";
    oss << "password='******', "; // 不显示实际密码
    
    oss << "createTime='" << FKUtils::format_time_point(_createTime) << "'";
    
    if (_updateTime) {
        oss << ", updateTime='" << FKUtils::format_time_point(*_updateTime) << "'";
    }
    
    oss << "}";
    return oss.str();
}