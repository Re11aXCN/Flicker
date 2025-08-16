#include "FKStatusServer.h"
#include "Library/Logger/logger.h"
#include "FKConfig.h"

#include <boost/asio.hpp>
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <format>

int main() {
    std::shared_ptr<FKStatusServer> server;
    boost::asio::io_context io_context;

    try {
        bool ok = Logger::getInstance().initialize("Flicker-StatusServer", Logger::SingleFile, true);
        if (!ok) return EXIT_FAILURE;

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        Flicker::Server::Config::StatusServer config{};
        server = std::make_shared<FKStatusServer>(io_context, config.getEndPoint());

        signals.async_wait([&](const boost::system::error_code& error, int signal_number) {
            if (error) {
                LOGGER_ERROR(std::format("信号处理错误: {}", error.message()));
                return;
            }
            
            LOGGER_INFO(std::format("接收到停止信号 ({}), 服务器正在关闭...", signal_number));
            server->stop();
            io_context.stop();
        });
        
        // 启动服务器
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
        LOGGER_INFO("状态服务器已安全关闭");
        Logger::getInstance().shutdown();
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("状态服务器异常: {}", e.what()));
        
        // 异常情况下也要确保服务器停止
        if (server) {
            server->stop();
        }
        
        return EXIT_FAILURE;
    }

    LOGGER_INFO("状态服务器已安全退出");
    return 0;
}
