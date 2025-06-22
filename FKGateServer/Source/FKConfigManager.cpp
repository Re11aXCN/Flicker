#include "FKConfigManager.h"
#include <fstream>
#include <print>
#include <filesystem>

SINGLETON_CREATE_CPP(FKConfigManager)
FKConfigManager::FKConfigManager()
{
    std::println("FKConfigManager 已创建");
    
    // 自动加载配置文件
    std::string configPath = "config.json";
    
    // 加载配置文件，如果加载失败则使用默认配置
    if (!loadFromFile(configPath)) {
        std::println("警告: 使用默认配置初始化服务器");
        // 保存默认配置到文件，以便后续修改
        saveToFile(configPath);
    }
}

FKConfigManager::~FKConfigManager()
{
    std::println("FKConfigManager 已销毁");
}

bool FKConfigManager::loadFromFile(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(_pConfigMutex);
    
    try {
        if (!std::filesystem::exists(filePath)) {
            std::println("配置文件 {} 不存在，将使用默认配置", filePath);
            return false;
        }
        
        std::ifstream configFile(filePath);
        if (!configFile.is_open()) {
            std::println("无法打开配置文件: {}", filePath);
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        if (!Json::parseFromStream(builder, configFile, &root, &errs)) {
            std::println("解析配置文件失败: {}", errs);
            return false;
        }
        
        // 读取服务器基本配置
        if (root.isMember("server")) {
            const Json::Value& server = root["server"];
            if (server.isMember("host")) _pServerConfig.Host = server["host"].asString();
            if (server.isMember("port")) _pServerConfig.Port = server["port"].asUInt();
            if (server.isMember("use_ssl")) _pServerConfig.UseSSL = server["use_ssl"].asBool();
        }
        
        // 读取ASIO线程池配置
        if (root.isMember("asio_thread_pool")) {
            const Json::Value& asioPool = root["asio_thread_pool"];
            if (asioPool.isMember("thread_count")) _pAsioThreadPoolConfig.ThreadCount = asioPool["thread_count"].asUInt();
            if (asioPool.isMember("channel_capacity")) _pAsioThreadPoolConfig.ChannelCapacity = asioPool["channel_capacity"].asUInt();
        }
        
        // 读取请求超时配置
        if (root.isMember("request_timeout")) {
            _pRequestTimeout = std::chrono::seconds(root["request_timeout"].asUInt());
        }
        
        // 读取gRPC服务配置
        if (root.isMember("grpc_services")) {
            const Json::Value& services = root["grpc_services"];
            for (const auto& serviceName : services.getMemberNames()) {
                // 将服务名称转换为枚举
                auto serviceTypeOpt = magic_enum::enum_cast<gRPC::ServiceType>(serviceName);
                if (!serviceTypeOpt.has_value()) {
                    std::println("未知的服务类型: {}", serviceName);
                    continue;
                }
                
                gRPC::ServiceType serviceType = serviceTypeOpt.value();
                const Json::Value& serviceConfig = services[serviceName];

				FKGrpcServiceConfig config;
                if (serviceConfig.isMember("host")) config.Host = serviceConfig["host"].asString();
                if (serviceConfig.isMember("port")) config.Port = serviceConfig["port"].asInt();
                if (serviceConfig.isMember("pool_size")) config.PoolSize = serviceConfig["pool_size"].asUInt();
                if (serviceConfig.isMember("use_ssl")) config.UseSSL = serviceConfig["use_ssl"].asBool();
                if (serviceConfig.isMember("timeout")) config.Timeout = std::chrono::seconds(serviceConfig["timeout"].asUInt());
                
                _pGrpcServiceConfigs[serviceType] = std::move(config);
            }
        }

		// 读取Redis配置
		if (root.isMember("redis")) {
			const Json::Value& redis = root["redis"];
			if (redis.isMember("host")) _pRedisConfig.Host = redis["host"].asString();
			if (redis.isMember("port")) _pRedisConfig.Port = redis["port"].asInt();
			if (redis.isMember("password")) _pRedisConfig.Password = redis["password"].asString();
            if (redis.isMember("pool_size")) _pRedisConfig.PoolSize = redis["pool_size"].asInt();
            if (redis.isMember("connection_timeout")) _pRedisConfig.ConnectionTimeout = std::chrono::milliseconds{ redis["connection_timeout"].asInt() };
			if (redis.isMember("socket_timeout")) _pRedisConfig.SocketTimeout = std::chrono::milliseconds{ redis["socket_timeout"].asInt() };
			if (redis.isMember("db_index")) _pRedisConfig.DBIndex = redis["db_index"].asInt();
        }
        
        // 读取数据库配置
        if (root.isMember("mysql")) {
            
        }

        std::println("成功加载配置文件: {}", filePath);
        return true;
    }
    catch (const std::exception& e) {
        std::println("加载配置文件异常: {}", e.what());
        return false;
    }
}

bool FKConfigManager::saveToFile(const std::string& filePath) const
{
    std::lock_guard<std::mutex> lock(_pConfigMutex);
    
    try {
        // 创建JSON根节点
        Json::Value root;
        
        // 服务器基本配置
        Json::Value server;
        server["host"] = _pServerConfig.Host;
        server["port"] = static_cast<Json::UInt>(_pServerConfig.Port);
        server["use_ssl"] = _pServerConfig.UseSSL;
        root["server"] = server;
        
        // ASIO线程池配置
        Json::Value asioPool;
        asioPool["thread_count"] = static_cast<Json::UInt>(_pAsioThreadPoolConfig.ThreadCount);
        asioPool["channel_capacity"] = static_cast<Json::UInt>(_pAsioThreadPoolConfig.ChannelCapacity);
        root["asio_thread_pool"] = asioPool;
        
        // 请求超时配置
        root["request_timeout"] = static_cast<Json::UInt>(_pRequestTimeout.count());
        
        // gRPC服务配置
        Json::Value grpcServices;
        for (const auto& [serviceType, config] : _pGrpcServiceConfigs) {
            auto serviceName = magic_enum::enum_name(serviceType);
            Json::Value serviceConfig;
            serviceConfig["host"] = config.Host;
            serviceConfig["port"] = static_cast<Json::UInt>(config.Port);
            serviceConfig["pool_size"] = static_cast<Json::UInt>(config.PoolSize);
            serviceConfig["use_ssl"] = config.UseSSL;
            serviceConfig["timeout"] = static_cast<Json::UInt>(config.Timeout.count());
            grpcServices[std::string(serviceName)] = serviceConfig;
        }
        root["grpc_services"] = grpcServices;

        // Redis配置
        Json::Value redis;
        redis["host"] = _pRedisConfig.Host;
        redis["port"] = static_cast<Json::UInt>(_pRedisConfig.Port);
        redis["password"] = _pRedisConfig.Password;
        redis["pool_size"] = static_cast<Json::UInt>(_pRedisConfig.PoolSize);
        redis["connection_timeout"] = static_cast<Json::UInt>(_pRedisConfig.ConnectionTimeout.count());
        redis["socket_timeout"] = static_cast<Json::UInt>(_pRedisConfig.SocketTimeout.count());
        redis["db_index"] = static_cast<Json::UInt>(_pRedisConfig.DBIndex);
        root["redis"] = redis;

        // 数据库配置
        // TODO: 数据库配置
        
        // 写入文件
        std::ofstream configFile(filePath);
        if (!configFile.is_open()) {
            std::println("无法打开配置文件进行写入: {}", filePath);
            return false;
        }
        
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "    "; // 使用4个空格缩进，使配置文件更易读
        std::unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
        writer->write(root, &configFile);
        
        std::println("成功保存配置到文件: {}", filePath);
        return true;
    }
    catch (const std::exception& e) {
        std::println("保存配置文件异常: {}", e.what());
        return false;
    }
}