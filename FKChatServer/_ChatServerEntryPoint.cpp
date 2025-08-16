#include <iostream>
#include <thread>
#include <signal.h>
#include <atomic>

#include <boost/asio/thread_pool.hpp>

#include "FKChatServer.h"
#include "universal/utils.h"
#include "Flicker/Global/FKConfig.h"
#include "Flicker/Global/Grpc/FKGrpcServiceStubPoolManager.h"
#include "Library/Logger/logger.h"

enum class ServerType {
    ChatMasterServer,
    ChatSlaveServer
};

int main(int argc, char* argv[]) {
    if (argc!= 1) {
        return EXIT_FAILURE;
    }
    ServerType serverType = ServerType::ChatMasterServer;
    //if (argv[0] == "ChatMasterServer") {
    //    serverType = ServerType::ChatMasterServer;
    //}
    //else if (argv[0] == "ChatSlaveServer") {
    //    serverType = ServerType::ChatSlaveServer;
    //}
    //else {
    //    return EXIT_FAILURE;
    //}

    std::shared_ptr<FKChatServer> server;
    boost::asio::io_context io_context;
    try {
        // 初始化日志系统
        bool ok = Logger::getInstance().initialize(universal::utils::string::concat("Flicker-", magic_enum::enum_name(serverType)), Logger::SingleFile, true);
        if (!ok) return EXIT_FAILURE;

        const auto& grpcManager = FKGrpcServiceStubPoolManager::getInstance();
        grpcManager->initializeService<Flicker::Server::Enums::GrpcServiceType::ValidateToken>(Flicker::Server::Config::ValidateTokenGrpcService{});

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        if (serverType == ServerType::ChatMasterServer) {
            Flicker::Server::Config::ChatMasterServer config{};
            server = std::make_shared<FKChatServer>(io_context, config.Host, config.Port, config.ID);
        }
        else if (serverType == ServerType::ChatSlaveServer) {
            Flicker::Server::Config::ChatSlaveServer config{};
            server = std::make_shared<FKChatServer>(io_context, config.Host, config.Port, config.ID);
        }

        signals.async_wait([&](const boost::system::error_code& error, int signal_number) {
            if (error) {
                LOGGER_ERROR(std::format("信号处理错误: {}", error.message()));
                return;
            }

            LOGGER_INFO(std::format("接收到停止信号 ({}), 服务器正在关闭...", signal_number));
            server->stop();
            io_context.stop();
            });

        server->start();
        // 在单独线程中运行信号处理
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

        LOGGER_INFO("聊天服务器已安全关闭");
        Logger::getInstance().shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("聊天服务器启动失败: {}", e.what()));
        return 1;
    }
}