#include <functional>
#include <iostream>
#include "Core/FKGateServer.h"

#include "Flicker/Global/Grpc/FKGrpcServiceStubPoolManager.h"
#include "Flicker/Global/FKDef.h"
#include "Flicker/Global/FKConfig.h"
#include "Flicker/Global/universal/mysql/connection_pool.h"

#include "Library/Logger/logger.h"

int main(int argc, char* argv[])
{
    std::shared_ptr<FKGateServer> server;
    try
    {
        bool ok = Logger::getInstance().initialize("Flicker-GateServer", Logger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;

        const auto& grpcManager = FKGrpcServiceStubPoolManager::getInstance();
        grpcManager->initializeService<Flicker::Server::Enums::GrpcServiceType::GenerateToken>(Flicker::Server::Config::GenerateTokenGrpcService{});
        
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        // 创建服务器实例并启动
        Flicker::Server::Config::GateServer gateServerConfig;
        server = std::make_shared<FKGateServer>(io_context, gateServerConfig.Port);
        signals.async_wait([&](const boost::system::error_code& error, int signalNumber) {
            if (error)
            {
                LOGGER_ERROR(std::format("Error: {}", error.message()));
                return;
            }
            LOGGER_INFO("接收到停止信号，服务器正在关闭...");

            server->stop();
            io_context.stop();
            });
        
        server->start();
        std::thread signal_thread([&io_context] {
            try {
                io_context.run();
            }
            catch (const std::exception& e) {
                LOGGER_ERROR(std::format("信号处理线程异常: {}", e.what()));
            }
            });

        // 确保信号线程正常结束
        if (signal_thread.joinable()) {
            signal_thread.join();
        }
        LOGGER_INFO("网关服务器已安全关闭");
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