#include <functional>
#include <iostream>
#include <json/json.h>
#include "Common/logger/logger_defend.h"
#include "Common/config/config_manager.h"
#include "FKGateServer.h"

int main(int argc, char* argv[])
{
    try
    {
        LOGGER_TRACE("活动连接数: {}", 1);

        bool ok = Logger::getInstance().initialize("Flicker-Server", Logger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;

        // 获取服务器配置（单例构造函数中已自动加载配置）
        auto serverConfig = ConfigManager::getInstance()->getServerConfig();
        LOGGER_INFO(std::format("服务器配置: {} 启动中...", serverConfig.getAddress()));
        
        boost::asio::io_context ioContext;
        boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);

        // 创建服务器实例并启动
        auto server = std::make_shared<FKGateServer>(ioContext, serverConfig.Port);
        signals.async_wait([&ioContext, &server](const boost::system::error_code& error, int signalNumber) {
            if (error)
            {
                LOGGER_ERROR(std::format("Error: {}", error.message()));
                return;
            }
            LOGGER_INFO("接收到停止信号，服务器正在关闭...");
            server->stop();

            ioContext.stop();
            });
        
        server->start();
        ioContext.run();

        Logger::getInstance().shutdown();
    }
    catch (const std::exception& e)
    {
        LOGGER_ERROR(std::format("异常: {}", e.what()));
        Logger::getInstance().shutdown();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}