/*************************************************************************************
 *
 * @ Filename	 : FKServerConfig.h
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

// 服务器配置类，使用单例模式
class FKServerConfig {
    SINGLETON_CREATE_H(FKServerConfig)

public:
    // ASIO线程池配置结构体
    struct AsioThreadPoolConfig {
        size_t threadCount{std::thread::hardware_concurrency() / 2}; // 默认为系统核心数的一半
        size_t channelCapacity{1024}; // 默认为1024
    };
	struct RedisConfig {
		std::string host{ "localhost" };
		int port{ 6379 };
		std::string password{};
	};
    
    bool loadFromFile(const std::string& filePath);
    bool saveToFile(const std::string& filePath) const;
    
    // 获取服务器基本配置
    const std::string& getServerHost() const { return _pServerHost; }
    uint16_t getServerPort() const { return _pServerPort; }
    bool getUseSSL() const { return _pUseSSL; }
    
    // 设置服务器基本配置
    void setServerHost(const std::string& host) { _pServerHost = host; }
    void setServerPort(uint16_t port) { _pServerPort = port; }
    void setUseSSL(bool useSSL) { _pUseSSL = useSSL; }
    
    // ASIO线程池配置
    const AsioThreadPoolConfig& getAsioThreadPoolConfig() const { return _pAsioThreadPoolConfig; }
    void setAsioThreadPoolConfig(const AsioThreadPoolConfig& config) { _pAsioThreadPoolConfig = config; }
    
    // 获取GRPC服务配置
    const FKGrpcServiceConfig& getGrpcServiceConfig(gRPC::ServiceType type) const {
        std::lock_guard<std::mutex> lock(_pConfigMutex);
        return _pGrpcServiceConfigs.at(type);
    }
    
    // 设置GRPC服务配置
    void setGrpcServiceConfig(gRPC::ServiceType type, const FKGrpcServiceConfig& config) {
        std::lock_guard<std::mutex> lock(_pConfigMutex);
        _pGrpcServiceConfigs[type] = config;
    }
    
    // 获取超时配置
    std::chrono::seconds getRequestTimeout() const { return _pRequestTimeout; }
    void setRequestTimeout(std::chrono::seconds timeout) { _pRequestTimeout = timeout; }
    
    // 获取数据库配置（预留接口，后续扩展）
    const std::string& getMysqlConnectionString() const { return _pMysqlConnectionString; }
    void setMysqlConnectionString(const std::string& connStr) { _pMysqlConnectionString = connStr; }
    
    // 获取Redis配置
    const RedisConfig& getRedisConfig() const { return _pRedisConfig; }
    void setRedisConfig(const RedisConfig& config) { _pRedisConfig = config; }
    
    // Redis配置的便捷访问方法
    const std::string& getRedisHost() const { return _pRedisConfig.host; }
    int getRedisPort() const { return _pRedisConfig.port; }
    const std::string& getRedisPassword() const { return _pRedisConfig.password; }
    
    void setRedisHost(const std::string& host) { _pRedisConfig.host = host; }
    void setRedisPort(int port) { _pRedisConfig.port = port; }
    void setRedisPassword(const std::string& password) { _pRedisConfig.password = password; }
    
private:
    FKServerConfig();
    ~FKServerConfig();
    
    // 服务器基本配置
    std::string _pServerHost{"localhost"};
    uint16_t _pServerPort{8080};
    bool _pUseSSL{false};
    
    // ASIO线程池配置
    AsioThreadPoolConfig _pAsioThreadPoolConfig{std::thread::hardware_concurrency() / 2, 1024};
    
    // GRPC服务配置
    std::unordered_map<gRPC::ServiceType, FKGrpcServiceConfig> _pGrpcServiceConfigs;
    
    // 超时配置
    std::chrono::seconds _pRequestTimeout{10};
    
    // 数据库配置（预留）
    std::string _pMysqlConnectionString;

    // Redis配置
    RedisConfig _pRedisConfig;
    
    // 互斥锁，保证线程安全
    mutable std::mutex _pConfigMutex;
};

#endif // !FK_SERVER_CONFIG_H_