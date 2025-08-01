#ifndef STRUCT_CONFIG_H_
#define STRUCT_CONFIG_H_

#include <chrono>
#include <string>

struct AsioIocThreadPoolConfig {
    uint16_t ThreadCount{ static_cast<uint16_t>(std::thread::hardware_concurrency() / 2) }; // 默认为系统核心数的一半
    uint16_t ChannelCapacity{ 1024 }; // 默认为1024
};

struct GateServerConfig {
    std::string Host{ "localhost" };
    uint16_t Port{ 8080 };
    bool UseSSL{ false };
    std::string getAddress() const {
        return std::format("{}:{}", Host, Port);
    }
};

struct GrpcServiceConfig {
    std::string Host{ "localhost" };
    uint16_t Port{ 50051 };
    uint16_t PoolSize{ 10 };
    bool UseSSL{ false };
    bool KeepAlivePermitWithoutCalls{ true };
    bool Http2MaxPingWithoutData{ false };
    std::chrono::milliseconds KeepAliveTime{ 30000 };
    std::chrono::milliseconds KeepAliveTimeout{ 10000 };
    std::chrono::milliseconds MaxReconnectBackoff{ 10000 };
    std::chrono::milliseconds GrpclbCallTimeout{ 2000 };
    std::string getAddress() const {
        return std::format("{}:{}", Host, Port);
    }
};

struct RedisConfig {
    std::string Host{ "localhost" };
    std::string Password{};
    std::chrono::milliseconds ConnectionTimeout{ 200 }; // 连接超时时间
    std::chrono::milliseconds SocketTimeout{ 100 };    // Socket超时时间
    uint16_t Port{ 6379 };
    uint16_t PoolSize{ 10 };
    uint16_t DBIndex{ 0 };                              // 数据库索引
};

// MySQL连接信息结构体
struct MySQLConfig {
    std::string Host{ "localhost" };
    uint16_t Port{ 3306 }; // C 3306  C++/xdevapi 33060
    uint16_t PoolSize{ 200 };
    std::string Username{ "root" };
    std::string Password{ "123456" };
    std::string Schema{ "flicker" };
    std::chrono::milliseconds ConnectionTimeout{ 500 }; // 连接超时时间
    std::chrono::milliseconds IdleTimeout{ 1800000 };     // 空闲连接超时时间
    std::chrono::milliseconds MonitorInterval{ 300000 };  // 监控线程检查间隔
};
#endif // !STRUCT_CONFIG_H_