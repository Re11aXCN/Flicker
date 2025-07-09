#include <print>
#include <functional>
#include <iostream>
#include <json/json.h>

#include "Asio/FKServer.h"
#include "Source/FKConfigManager.h"


class Finally {
public:
	explicit Finally(std::function<void()> action)
		: action_(std::move(action)) {
	}

	~Finally() noexcept {
		if (action_) {
			try { action_(); }
			catch (...) { /* 禁止抛出异常 */ }
		}
	}

	Finally(const Finally&) = delete;
	Finally& operator=(const Finally&) = delete;

private:
	std::function<void()> action_;
};

#include <mysqlx/xdevapi.h>
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define _finally(code) \
    Finally CONCAT(_finally_, __LINE__)([&]() ##code);
int main(int argc, char* argv[])
{
	//mysqlx::SessionSettings option("localhost", 33060, "root", "123456", "flicker");
	//mysqlx::Session session(option);
	//mysqlx::Schema sch = session.getSchema("flicker");
	//mysqlx::SqlResult res = session.sql("SELECT 1").execute();
	//std::cout << "Result: " << res.count();
	//session.close();
	try
	{
		// 获取服务器配置（单例构造函数中已自动加载配置）
		auto serverConfig = FKConfigManager::getInstance()->getServerConfig();
		std::println("服务器: {} 启动中...", serverConfig.getAddress());
		
		boost::asio::io_context ioContext;
		boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
		signals.async_wait([&ioContext](const boost::system::error_code& error, int signalNumber) {
			if (error)
			{
				std::println("Error: {}", error.message());
				return;
			}
			std::println("接收到停止信号，服务器正在关闭...");
			ioContext.stop();
			});
		
		// 使用配置创建服务器
		std::make_shared<FKServer>(ioContext, serverConfig.Port)->start();
		ioContext.run();
	}
	catch (const std::exception& e)
	{
		std::println("EXCEPTION: {}", e.what());
		return EXIT_FAILURE;
	}
}
