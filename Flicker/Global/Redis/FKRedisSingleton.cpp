#include "FKRedisSingleton.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Library/Logger/logger.h"

SINGLETON_CREATE_CPP(FKRedisSingleton)
FKRedisSingleton::FKRedisSingleton()
{
    RedisConfig config;
    try {
        sw::redis::ConnectionOptions opts;
        opts.db = config.DBIndex;
        opts.host = config.Host;
        opts.port = config.Port;
        opts.password = config.Password;
        opts.keep_alive = config.KeepAlive;
        opts.keep_alive_s = config.KeepAliveS;
        opts.connect_timeout = config.ConnectTimeoutMs;
        opts.socket_timeout = config.SocketTimeoutMs;
        sw::redis::ConnectionPoolOptions pool_opts;
        pool_opts.size = config.PoolSize;
        pool_opts.wait_timeout = config.WaitTimeoutMs;
        pool_opts.connection_lifetime = config.ConnectionLifetimeMin;
        pool_opts.connection_idle_time = config.ConnectionIdleTimeMin;

        _redis = std::make_unique<sw::redis::Redis>(opts, pool_opts);
    }
    catch (const sw::redis::Error& e) {
        LOGGER_ERROR(std::format("Redis connection failed: {}", e.what()));
    }
}

RedisResult<long long> FKRedisSingleton::ttl(const std::string& key) {
    if (!_redis) {
        return std::unexpected(RedisError{ RedisErrorCode::ConnectionFailed, "Redis connection not established" });
    }

    try {
        return _redis->ttl(key);
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("TTL operation failed: {}", e.what())
            });
    }
}

RedisResult<std::vector<std::string>> FKRedisSingleton::scan(const std::string& pattern) {
    if (!_redis) {
        return std::unexpected(RedisError{
            RedisErrorCode::ConnectionFailed,
            "Redis connection not established"
            });
    }

    try {
        std::vector<std::string> keys;
        sw::redis::Cursor cursor = 0;  // 使用 Redis++ 的 Cursor 类型
        constexpr long long SCAN_COUNT = 100;  // 每次扫描的数量

        do {
            // 使用正确的 scan 接口：cursor, pattern, count, 输出迭代器
            cursor = _redis->scan(cursor, pattern, SCAN_COUNT, std::back_inserter(keys));
        } while (cursor != 0);  // 当 cursor 返回 0 时表示扫描完成

        return keys;
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("SCAN operation failed: {}", e.what())
            });
    }
}

RedisResult<bool> FKRedisSingleton::exists(const std::string& key) {
    if (!_redis) {
        return std::unexpected(RedisError{ RedisErrorCode::ConnectionFailed, "Redis connection not established" });
    }

    try {
        return _redis->exists(key) > 0;
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("EXISTS operation failed: {}", e.what())
            });
    }
}

RedisResult<bool> FKRedisSingleton::del(const std::string& key) {
    if (!_redis) {
        return std::unexpected(RedisError{ RedisErrorCode::ConnectionFailed, "Redis connection not established" });
    }

    try {
        return _redis->del(key) > 0;
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("DEL operation failed: {}", e.what())
            });
    }
}

RedisResult<std::string> FKRedisSingleton::get(const std::string& key) {
    if (!_redis) {
        return std::unexpected(RedisError{ RedisErrorCode::ConnectionFailed, "Redis connection not established" });
    }

    try {
        auto val = _redis->get(key);
        if (!val) {
            return std::unexpected(RedisError{
                RedisErrorCode::KeyNotFound,
                "Key does not exist"
                });
        }
        return *val;
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("GET operation failed: {}", e.what())
            });
    }
}

RedisResult<bool> FKRedisSingleton::set(const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl) {
    if (!_redis) {
        return std::unexpected(RedisError{ RedisErrorCode::ConnectionFailed, "Redis connection not established" });
    }

    try {
        if (ttl.count() > 0) {
            _redis->setex(key, ttl, value);
        }
        else {
            _redis->set(key, value);
        }
        return true;
    }
    catch (const sw::redis::Error& e) {
        return std::unexpected(RedisError{
            RedisErrorCode::OperationFailed,
            std::format("SET operation failed: {}", e.what())
            });
    }
}

std::string FKRedisSingleton::_generateCode() {
    static boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid).substr(0, 6);
}

RedisResult<std::string> FKRedisSingleton::generateAndStoreCode(const std::string& email) {
    auto* instance = getInstance();

    std::string key = _getKey(email);

    // 1. 先检查是否已有验证码
    auto existingCode = instance->get(key);
    if (existingCode) {
        // 已有验证码，重用并记录日志
        LOGGER_WARN(std::format("User {} requested new code but existing code found. Reusing code {}.", email, *existingCode));
        return existingCode;
    }

    // 2. 只有KeyNotFound错误才生成新验证码（其他错误直接返回）
    if (!existingCode && existingCode.error().code != RedisErrorCode::KeyNotFound) {
        LOGGER_ERROR(std::format("Failed to check if user {} has existing code: {}", email, existingCode.error().message));
        return std::unexpected(existingCode.error());
    }

    // 3. 生成新验证码并存储
    std::string code = _generateCode();
    auto result = instance->set(key, code, DEFAULT_TTL_5M);
    if (!result) {
        LOGGER_ERROR(std::format("Failed to store verification code for user {}: {}", email, result.error().message));
        return std::unexpected(result.error());
    }
    LOGGER_DEBUG(std::format("Generated new verification code for {}, code is {}", email, code));
    return code;
}

RedisResult<bool> FKRedisSingleton::verifyCode(const std::string& email, const std::string& code) {
    auto* instance = getInstance();

    std::string key = _getKey(email);

    // 获取存储的验证码
    auto storedCode = instance->get(key);
    if (!storedCode) {
        if (storedCode.error().code == RedisErrorCode::KeyNotFound) {
            LOGGER_WARN(std::format("User {} requested verification code but no code found. Code expired or invalid.", email));
            return std::unexpected(RedisError{ RedisErrorCode::ValueExpired, "Verification code expired or does not exist" });
        }
        LOGGER_ERROR(std::format("Failed to get verification code for user {}: {}", email, storedCode.error().message));
        return std::unexpected(storedCode.error());
    }

    // 比较验证码
    if (*storedCode != code) {
        LOGGER_WARN(std::format("User {} entered invalid verification code. Code expired or invalid.", email));
        return std::unexpected(RedisError{ RedisErrorCode::ValueMismatch,"Verification code mismatch" });
    }

    // 验证成功，删除验证码
    auto delResult = instance->del(key);
    if (!delResult) {
        LOGGER_ERROR(std::format("Failed to delete verification code for user {}: {}", email, delResult.error().message));
        return std::unexpected(delResult.error());
    }

    return true;
}

RedisResult<bool> FKRedisSingleton::storeToken(const std::string& token, const std::string& user_uuid, std::chrono::seconds ttl) {
    auto* instance = getInstance();

    auto tokenKey = _getTokenKey(token);
    auto existingToken = instance->get(tokenKey);
    if (existingToken) {
        LOGGER_WARN(std::format("Token {} requested new code but existing code found. Reusing code {}.", token, *existingToken));
        return true;
    }

    if (!existingToken && existingToken.error().code != RedisErrorCode::KeyNotFound) {
        LOGGER_ERROR(std::format("Failed to get token for user {}: {}", user_uuid, existingToken.error().message));
        return false;
    }

    auto result = instance->set(tokenKey, user_uuid, ttl);
    if (!result) {
        LOGGER_ERROR(std::format("Failed to store token for user {}: {}", user_uuid, result.error().message));
        return false;
    }
    LOGGER_DEBUG(std::format("Token stored successfully for user: {}", user_uuid));
    return true;
}

RedisResult<std::string> FKRedisSingleton::getTokenUser(const std::string& token) {
    auto* instance = getInstance();

    auto val = instance->get(_getTokenKey(token));
    if (!val) {
        LOGGER_ERROR(std::format("Failed to get token user: {}", val.error().message));
        return std::unexpected(val.error());
    }
    LOGGER_DEBUG(std::format("Token user found: {}", *val));
    return *val;
}

RedisResult<bool> FKRedisSingleton::deleteToken(const std::string& token) {
    auto* instance = getInstance();

    auto val = instance->del(_getTokenKey(token));
    if (!val) {
        LOGGER_ERROR(std::format("Failed to delete token: {}", val.error().message));
        return std::unexpected(val.error());
    }
    LOGGER_DEBUG(std::format("Token deleted successfully: {}", token));
    return *val;
}

RedisResult<bool> FKRedisSingleton::isTokenValid(const std::string& token) {
    auto* instance = getInstance();

    auto val = instance->exists(_getTokenKey(token));
    if (!val) {
        LOGGER_ERROR(std::format("Failed to check token validity: {}", val.error().message));
        return std::unexpected(val.error());
    }
    LOGGER_DEBUG(std::format("Token validity check result: {}", *val));
    return *val;
}

RedisResult<int64_t> FKRedisSingleton::cleanupExpiredTokens() {
    auto* instance = getInstance();

    // 1. 扫描所有 token 键
    auto keysResult = instance->scan(_getTokenKey("*"));
    if (!keysResult) {
        return std::unexpected(keysResult.error());
    }

    const auto& keys = *keysResult;
    int64_t deletedCount = 0;

    // 2. 检查每个键的 TTL 并删除过期项
    for (const auto& fullKey : keys) {
        // 获取 TTL（生存时间）
        long long ttl = -3;  // 初始化为无效值

        auto ttlRes = instance->ttl(fullKey);  // 直接获取 TTL 值
        if (!ttlRes) {
            LOGGER_ERROR(std::format("Failed to get TTL for key {}: {}", fullKey, ttlRes.error().message));
            continue;
        }
        ttl = *ttlRes;

        // 3. 删除已过期或未设置过期时间的键
        if (ttl == -2 || ttl == -1) {  // -2: 不存在, -1: 永不过期
            auto delRes = instance->del(fullKey);
            if (!delRes) {
                LOGGER_ERROR(std::format("Failed to delete key {}: {}", fullKey, delRes.error().message));
                continue;
            }
            deletedCount++;
            LOGGER_DEBUG(std::format("Cleaned expired key: {}", fullKey));
        }
    }

    LOGGER_INFO(std::format("Cleaned {} expired tokens", deletedCount));
    return deletedCount;
}
