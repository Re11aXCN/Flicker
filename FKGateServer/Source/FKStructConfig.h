#ifndef FK_STRUCT_CONFIG_H_
#define FK_STRUCT_CONFIG_H_
#include <string>
#include <chrono>
#include <thread>
struct FKAsioThreadPoolConfig {
	uint16_t ThreadCount{ static_cast<uint16_t>(std::thread::hardware_concurrency() / 2) }; // 默认为系统核心数的一半
	uint16_t ChannelCapacity{ 1024 }; // 默认为1024
};

struct FKServerConfig {
	std::string Host{ "localhost" };
	uint16_t Port{ 8080 };
	bool UseSSL{ false };
	std::string getAddress() const {
		return std::format("{}:{}", Host, Port);
	}
};

struct FKGrpcServiceConfig {
	std::string Host{ "localhost" };
	uint16_t Port{ 50051 };
	uint16_t PoolSize{ 10 };
	bool UseSSL{ false };
	std::chrono::seconds Timeout{ 10 };
	std::string getAddress() const {
		return std::format("{}:{}", Host, Port);
	}
};

struct FKRedisConfig {
	std::string Host{ "localhost" };
	std::string Password{};
	std::chrono::milliseconds ConnectionTimeout{ 100 }; // 连接超时时间
	std::chrono::milliseconds SocketTimeout{ 100 };    // Socket超时时间
	uint16_t Port{ 6379 };
	uint16_t PoolSize{ 10 };
	uint16_t DBIndex{ 0 };                              // 数据库索引
};

// MySQL连接信息结构体
struct FKMySQLConfig {
	std::string Host{ "localhost" };
	uint16_t Port{ 3306 };
	uint16_t PoolSize{ 200 };
	std::string Username{ "root" };
	std::string Password{};
	std::string Schema{ "flicker" };
	std::chrono::seconds ConnectionTimeout{ 10 }; // 连接超时时间
	std::chrono::seconds IdleTimeout{ 300 };     // 空闲连接超时时间
	std::chrono::seconds MonitorInterval{ 60 };  // 监控线程检查间隔
};
#endif // !FK_STRUCT_CONFIG_H_