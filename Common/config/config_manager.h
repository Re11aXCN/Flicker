/*************************************************************************************
 *
 * @ Filename     : ConfigManager.h
 * @ Description : 服务器配置单例类，集中管理服务器的各种配置参数
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
#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>

#include "Common/global/macro.h"
#include "Common/global/define_enum.h"
#include "Common/config/struct_config.h"

struct ConfigManager {
    SINGLETON_CREATE_H(ConfigManager)

public:
    bool loadFromFile(const std::string& filePath);
    bool saveToFile(const std::string& filePath) const;

    const GateServerConfig& getServerConfig() const { return _pServerConfig; }
    const AsioIocThreadPoolConfig& getAsioIocThreadPoolConfig() const { return _pAsioThreadPoolConfig; }
    const GrpcServiceConfig& getGrpcServiceConfig(flicker::grpc::service type) const { return _pGrpcServiceConfigs.at(type); }
    const RedisConfig& getRedisConfig() const { return _pRedisConfig; }
    const MySQLConfig& getMySQLConnectionString() const { return _pMySQLConfig; }
    const std::chrono::milliseconds& getRequestTimeout() const { return _pRequestTimeout; }

private:
    ConfigManager();
    ~ConfigManager();
    
    // 服务器基本配置
    GateServerConfig _pServerConfig;
    
    // ASIO线程池配置
    AsioIocThreadPoolConfig _pAsioThreadPoolConfig;
    
    // GRPC服务配置
    std::unordered_map<flicker::grpc::service, GrpcServiceConfig> _pGrpcServiceConfigs;
    
    // Redis配置
    RedisConfig _pRedisConfig;

    // 数据库配置
    MySQLConfig _pMySQLConfig;

    // 超时配置
    std::chrono::milliseconds _pRequestTimeout{ 10 };

    // 互斥锁，保证线程安全
    mutable std::mutex _pConfigMutex;
};

#endif // !CONFIG_MANAGER_H_