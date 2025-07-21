
#include <functional>
#include <iostream>
#include <json/json.h>
#include <mysql.h>

#include "Asio/FKServer.h"
#include "Source/FKConfigManager.h"
#include "FKLogger.h"
#include "MySQL/FKMySQLConnectionPool.h"
int main(int argc, char* argv[])
{
    try
    {
        bool ok = FKLogger::getInstance().initialize("Flicker-Server", FKLogger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;

        FKMySQLConnectionPool::getInstance()->executeWithConnection([](MYSQL* mysql) {
            try {
                // 创建用户表SQL
                std::string createTableSQL = R"(
CREATE TABLE IF NOT EXISTS
user (
id INT AUTO_INCREMENT PRIMARY KEY,
uuid VARCHAR(36) NOT NULL UNIQUE,
username VARCHAR(30) NOT NULL UNIQUE,
email VARCHAR(320) NOT NULL UNIQUE,
password VARCHAR(60) NOT NULL, 
create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
update_time TIMESTAMP NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP,
INDEX idx_email (email),
INDEX idx_username (username)
)
ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
)";
// 执行创建用户表SQL
                if (mysql_query(mysql, createTableSQL.c_str())) {
                    FK_SERVER_ERROR(std::format("创建用户表失败: {}", mysql_error(mysql)));
                    return false;
                }
                return true;
            }
            catch (const std::exception& e) {
                return false;
            }
            });

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
