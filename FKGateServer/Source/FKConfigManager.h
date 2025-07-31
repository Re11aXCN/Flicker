/*************************************************************************************
 *
 * @ Filename     : FKConfigManager.h
 * @ Description : 服务器配置单例类，集中管理服务器的各种配置参数
 *                 用于
 *                 FKAsioThreadPool 单例构造时配置初始化、
 *                 FKGrpcServicePoolManager 单例构造时 注册连接池服务初始化FKGrpcServiceConfig
 *                 Redis 预留
 *                 Mysql 预留
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
#ifndef FK_SERVER_CONFIG_H_
#define FK_SERVER_CONFIG_H_

#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>

#include "FKMacro.h"
#include "FKDef.h"
#include "FKStructConfig.h"

struct FKConfigManager {
    SINGLETON_CREATE_H(FKConfigManager)

public:
    bool loadFromFile(const std::string& filePath);
    bool saveToFile(const std::string& filePath) const;

    const FKServerConfig& getServerConfig() const { return _pServerConfig; }
    const FKAsioThreadPoolConfig& getAsioThreadPoolConfig() const { return _pAsioThreadPoolConfig; }
    const FKGrpcServiceConfig& getGrpcServiceConfig(flicker::grpc::service type) const { return _pGrpcServiceConfigs.at(type); }
    const FKRedisConfig& getRedisConfig() const { return _pRedisConfig; }
    const FKMySQLConfig& getMySQLConnectionString() const { return _pMySQLConfig; }
    const std::chrono::milliseconds& getRequestTimeout() const { return _pRequestTimeout; }

private:
    FKConfigManager();
    ~FKConfigManager();
    
    // 服务器基本配置
    FKServerConfig _pServerConfig;
    
    // ASIO线程池配置
    FKAsioThreadPoolConfig _pAsioThreadPoolConfig;
    
    // GRPC服务配置
    std::unordered_map<flicker::grpc::service, FKGrpcServiceConfig> _pGrpcServiceConfigs;
    
    // Redis配置
    FKRedisConfig _pRedisConfig;

    // 数据库配置
    FKMySQLConfig _pMySQLConfig;

    // 超时配置
    std::chrono::milliseconds _pRequestTimeout{ 10 };

    // 互斥锁，保证线程安全
    mutable std::mutex _pConfigMutex;
};

#endif // !FK_SERVER_CONFIG_H_