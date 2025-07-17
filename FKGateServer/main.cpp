
#include <functional>
#include <iostream>
#include <json/json.h>

#include "Asio/FKServer.h"
#include "Source/FKConfigManager.h"
#include "FKLogger.h"
int main(int argc, char* argv[])
{
    try
    {
        bool ok = FKLogger::getInstance().initialize("Flicker-Server", FKLogger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;

        // 获取服务器配置（单例构造函数中已自动加载配置）
        auto serverConfig = FKConfigManager::getInstance()->getServerConfig();
        FK_SERVER_INFO(std::format("服务器配置: {} 启动中...", serverConfig.getAddress()));
        
        boost::asio::io_context ioContext;
        boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);

        // 创建服务器实例并启动
        auto server = std::make_shared<FKServer>(ioContext, serverConfig.Port);
        signals.async_wait([&ioContext, &server](const boost::system::error_code& error, int signalNumber) {
            if (error)
            {
                FK_SERVER_ERROR(std::format("Error: {}", error.message()));
                return;
            }
            FK_SERVER_INFO("接收到停止信号，服务器正在关闭...");
            server->stop();

            ioContext.stop();
            });
        
        server->start();
        ioContext.run();

        FKLogger::getInstance().shutdown();
    }
    catch (const std::exception& e)
    {
        FK_SERVER_ERROR(std::format("异常: {}", e.what()));
        FKLogger::getInstance().shutdown();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
