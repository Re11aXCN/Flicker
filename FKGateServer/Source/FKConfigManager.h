/*************************************************************************************
 *
 * @ Filename	 : FKConfigManager.h
 * @ Description : 服务器配置单例类，集中管理服务器的各种配置参数
 *                 用于
 *                 FKAsioThreadPool 单例构造时配置初始化、
 *                 FKGrpcServiceManager 单例构造时 注册连接池服务初始化FKGrpcServiceConfig
 *                 Redis 预留
 *                 Mysql 预留
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
#ifndef FK_SERVER_CONFIG_H_
#define FK_SERVER_CONFIG_H_

#include <string>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <mutex>
#include <json/json.h>
#include "FKMacro.h"
#include "FKDef.h"
#include "Grpc/FKGrpcConnectionPool.hpp"

#pragma region CONFIG_STRUCT
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
#pragma endregion CONFIG_STRUCT



// 服务器配置类，使用单例模式
struct FKConfigManager {
    SINGLETON_CREATE_H(FKConfigManager)

public:
    bool loadFromFile(const std::string& filePath);
    bool saveToFile(const std::string& filePath) const;

    const FKServerConfig& getServerConfig() const { return _pServerConfig; }
	const FKAsioThreadPoolConfig& getAsioThreadPoolConfig() const { return _pAsioThreadPoolConfig; }
	const FKGrpcServiceConfig& getGrpcServiceConfig(gRPC::ServiceType type) const { return _pGrpcServiceConfigs.at(type); }
	const FKRedisConfig& getRedisConfig() const { return _pRedisConfig; }
	const FKMySQLConfig& getMySQLConnectionString() const { return _pMySQLConfig; }
	const std::chrono::seconds& getRequestTimeout() const { return _pRequestTimeout; }

private:
    FKConfigManager();
    ~FKConfigManager();
    
    // 服务器基本配置
    FKServerConfig _pServerConfig;
    
    // ASIO线程池配置
    FKAsioThreadPoolConfig _pAsioThreadPoolConfig;
    
    // GRPC服务配置
    std::unordered_map<gRPC::ServiceType, FKGrpcServiceConfig> _pGrpcServiceConfigs;
    
	// Redis配置
	FKRedisConfig _pRedisConfig;

    // 数据库配置
	FKMySQLConfig _pMySQLConfig;

	// 超时配置
	std::chrono::seconds _pRequestTimeout{ 10 };

    // 互斥锁，保证线程安全
    mutable std::mutex _pConfigMutex;
};

#endif // !FK_SERVER_CONFIG_H_