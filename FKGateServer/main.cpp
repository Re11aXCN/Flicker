#include <print>
#include <json/json.h>

#include "FKServer.h"
#include "Source/FKServerConfig.h"
#include <functional>
#include <iostream>

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


#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define _finally(code) \
    Finally CONCAT(_finally_, __LINE__)([&]() ##code);
int main(int argc, char* argv[])
{
	try
	{
		// 获取服务器配置（单例构造函数中已自动加载配置）
		auto serverConfig = FKServerConfig::getInstance();
		
		// 获取服务器配置
		UINT16 port = serverConfig->getServerPort();
		size_t threadPoolSize = serverConfig->getAsioThreadPoolSize();
		
		std::println("服务器启动中...");
		std::println("端口: {}", port);
		std::println("ASIO线程池大小: {}", threadPoolSize);
		
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
		std::make_shared<FKServer>(ioContext, port)->start();
		ioContext.run();
	}
	catch (const std::exception& e)
	{
		std::println("EXCEPTION: {}", e.what());
		return EXIT_FAILURE;
	}
}
