#include "FKServerConfig.h"
#include <fstream>
#include <print>
#include <filesystem>

SINGLETON_CREATE_CPP(FKServerConfig)
FKServerConfig::FKServerConfig()
{
    std::println("FKServerConfig 已创建");
    
    // 自动加载配置文件
    std::string configPath = "config.json";
    
    // 加载配置文件，如果加载失败则使用默认配置
    if (!loadFromFile(configPath)) {
        std::println("警告: 使用默认配置初始化服务器");
        // 保存默认配置到文件，以便后续修改
        saveToFile(configPath);
    }
}

FKServerConfig::~FKServerConfig()
{
    std::println("FKServerConfig 已销毁");
}

bool FKServerConfig::loadFromFile(const std::string& filePath)
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
            if (server.isMember("host")) _pServerHost = server["host"].asString();
            if (server.isMember("port")) _pServerPort = server["port"].asUInt();
            if (server.isMember("useSSL")) _pUseSSL = server["useSSL"].asBool();
        }
        
        // 读取ASIO线程池配置
        if (root.isMember("asioThreadPool")) {
            const Json::Value& asioPool = root["asioThreadPool"];
            AsioThreadPoolConfig config;
            if (asioPool.isMember("threadCount")) config.threadCount = asioPool["threadCount"].asUInt();
            if (asioPool.isMember("channelCapacity")) config.channelCapacity = asioPool["channelCapacity"].asUInt();
            _pAsioThreadPoolConfig = config;
        }
        
        // 读取请求超时配置
        if (root.isMember("requestTimeout")) {
            _pRequestTimeout = std::chrono::seconds(root["requestTimeout"].asUInt());
        }
        
        // 读取gRPC服务配置
        if (root.isMember("grpcServices")) {
            const Json::Value& services = root["grpcServices"];
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
                if (serviceConfig.isMember("host")) config.host = serviceConfig["host"].asString();
                if (serviceConfig.isMember("port")) config.port = serviceConfig["port"].asInt();
                if (serviceConfig.isMember("poolSize")) config.poolSize = serviceConfig["poolSize"].asUInt();
                if (serviceConfig.isMember("useSSL")) config.useSSL = serviceConfig["useSSL"].asBool();
                if (serviceConfig.isMember("timeout")) config.timeout = std::chrono::seconds(serviceConfig["timeout"].asUInt());
                
                _pGrpcServiceConfigs[serviceType] = std::move(config);
            }
        }
        
        // 读取数据库配置
        if (root.isMember("mysql")) {
            const Json::Value& mysql = root["mysql"];
            if (mysql.isMember("connectionString")) _pMysqlConnectionString = mysql["connectionString"].asString();
        }
        
        // 读取Redis配置
        if (root.isMember("redis")) {
            const Json::Value& redis = root["redis"];
            if (redis.isMember("host")) _pRedisConfig.host = redis["host"].asString();
            if (redis.isMember("port")) _pRedisConfig.port = redis["port"].asInt();
            if (redis.isMember("password")) _pRedisConfig.password = redis["password"].asString();
        }
        
        std::println("成功加载配置文件: {}", filePath);
        return true;
    }
    catch (const std::exception& e) {
        std::println("加载配置文件异常: {}", e.what());
        return false;
    }
}

bool FKServerConfig::saveToFile(const std::string& filePath) const
{
    std::lock_guard<std::mutex> lock(_pConfigMutex);
    
    try {
        // 创建JSON根节点
        Json::Value root;
        
        // 服务器基本配置
        Json::Value server;
        server["host"] = _pServerHost;
        server["port"] = _pServerPort;
        server["useSSL"] = _pUseSSL;
        root["server"] = server;
        
        // ASIO线程池配置
        Json::Value asioPool;
        asioPool["threadCount"] = static_cast<Json::UInt>(_pAsioThreadPoolConfig.threadCount);
        asioPool["channelCapacity"] = static_cast<Json::UInt>(_pAsioThreadPoolConfig.channelCapacity);
        root["asioThreadPool"] = asioPool;
        
        // 请求超时配置
        root["requestTimeout"] = static_cast<Json::UInt>(_pRequestTimeout.count());
        
        // gRPC服务配置
        Json::Value grpcServices;
        for (const auto& [serviceType, config] : _pGrpcServiceConfigs) {
            auto serviceName = magic_enum::enum_name(serviceType);
            Json::Value serviceConfig;
            serviceConfig["host"] = config.host;
            serviceConfig["port"] = config.port;
            serviceConfig["poolSize"] = static_cast<Json::UInt>(config.poolSize);
            serviceConfig["useSSL"] = config.useSSL;
            serviceConfig["timeout"] = static_cast<Json::UInt>(config.timeout.count());
            grpcServices[std::string(serviceName)] = serviceConfig;
        }
        root["grpcServices"] = grpcServices;
        
        // 数据库配置
        Json::Value mysql;
        mysql["connectionString"] = _pMysqlConnectionString;
        root["mysql"] = mysql;
        
        // Redis配置
        Json::Value redis;
        redis["host"] = _pRedisConfig.host;
        redis["port"] = _pRedisConfig.port;
        redis["password"] = _pRedisConfig.password;
        root["redis"] = redis;
        
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