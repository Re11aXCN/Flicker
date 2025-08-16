#ifndef FK_REDIS_SINGLETON_H_
#define FK_REDIS_SINGLETON_H_
#include <iostream>
#include <expected>
#pragma warning(push)
#pragma warning(disable:4200)
#include <sw/redis++/redis++.h>
#pragma warning(pop)
#include "universal/macros.h"

struct RedisConfig {
    std::string Host{ "127.0.0.1" };
    std::string Password{ "123456" };
    std::chrono::milliseconds ConnectTimeoutMs{ 200 };   // 连接超时时间
    std::chrono::milliseconds SocketTimeoutMs{ 100 };    // Socket超时时间
    std::chrono::milliseconds WaitTimeoutMs{ 200 };     // 等待超时200ms
    std::chrono::seconds KeepAliveS{ 60000 };
    std::chrono::minutes ConnectionLifetimeMin{ 30 };   // 连接最大生命周期30分钟
    std::chrono::minutes ConnectionIdleTimeMin{ 5 };    // 空闲连接超时5分钟
    uint16_t Port{ 6379 };
    uint16_t PoolSize{ 20 };
    uint16_t DBIndex{ 1 };                              // 数据库索引
    bool KeepAlive{ true };                             // 是否保持连接
};

enum class RedisErrorCode {
    ConnectionFailed,
    OperationFailed,
    KeyNotFound,
    ValueMismatch,
    ValueExpired
};

// Redis错误结构体
struct RedisError {
    RedisErrorCode code;
    std::string message;

    friend std::ostream& operator<<(std::ostream& os, const RedisError& err) {
        os << "RedisError[" << static_cast<int>(err.code) << "]: " << err.message;
        return os;
    }
};

template<typename T>
using RedisResult = std::expected<T, RedisError>;

class FKRedisSingleton
{
    SINGLETON_CREATE_H(FKRedisSingleton)
public:
    static constexpr std::chrono::seconds DEFAULT_TTL_5M = std::chrono::seconds(300);
    static constexpr std::chrono::seconds DEFAULT_TTL_1D = std::chrono::seconds(24 * 60 * 60);

    // 基本操作封装
    // 在类定义中添加以下方法声明
    RedisResult<long long> ttl(const std::string& key);
    RedisResult<std::vector<std::string>> scan(const std::string& pattern);
    RedisResult<bool> exists(const std::string& key);
    RedisResult<bool> del(const std::string& key);
    RedisResult<std::string> get(const std::string& key);
    RedisResult<bool> set(const std::string& key, const std::string& value,
        std::chrono::seconds ttl = std::chrono::seconds{ 0 });

    // 验证码操作
    static RedisResult<std::string> generateAndStoreCode(const std::string& email);
    static RedisResult<bool> verifyCode(const std::string& email, const std::string& code);

    // Token操作
    static RedisResult<bool> storeToken(const std::string& token, const std::string& user_uuid,
        std::chrono::seconds ttl = DEFAULT_TTL_1D); // 24小时
    static RedisResult<std::string> getTokenUser(const std::string& token);
    static RedisResult<bool> deleteToken(const std::string& token);
    static RedisResult<bool> isTokenValid(const std::string& token);

    // 清理过期Token（可选的维护操作）
    static RedisResult<int64_t> cleanupExpiredTokens();

private:
    FKRedisSingleton();
    ~FKRedisSingleton() = default;

    std::unique_ptr<sw::redis::Redis> _redis;
    static constexpr std::string_view VERIFICATION_PREFIX = "verification_code:";
    static constexpr std::string_view TOKEN_PREFIX = "token:";

    // 生成验证码
    static std::string _generateCode();

    // 获取完整键名
    static std::string _getKey(const std::string& email) {
        return std::string(VERIFICATION_PREFIX) + email;
    }

    // 获取Token键名
    static std::string _getTokenKey(const std::string& token) {
        return std::string(TOKEN_PREFIX) + token;
    }
};

#endif // !FK_REDIS_SINGLETON_H_