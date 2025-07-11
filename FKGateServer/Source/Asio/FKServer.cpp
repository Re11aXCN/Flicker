﻿#include "FKServer.h"

#include <print>

#include "Asio/FKHttpConnection.h"
#include "Asio/FKAsioThreadPool.h"
FKServer::FKServer(boost::asio::io_context& ioc, UINT16 port)
	: _pIoContext(ioc)
	, _pAcceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{

}

void FKServer::start()
{
	try {
		// 获取下一个可用的IO上下文
		FKAsioThreadPool::ioContext& ioc = FKAsioThreadPool::getInstance()->getNextContext();
		
		// 创建新的HTTP连接对象
		std::shared_ptr<FKHttpConnection> connection = std::make_shared<FKHttpConnection>(ioc);
		
		// 异步接受新连接
		_pAcceptor.async_accept(
			connection->getSocket(), 
			[self = shared_from_this(), connection](boost::beast::error_code ec) {
				try {
					// 处理接受连接时的错误
					if (ec) {
						std::println("接受连接错误: {}", ec.message());
						
						// 短暂延迟后继续监听，避免在错误情况下的快速循环
						boost::asio::steady_timer timer(self->_pIoContext);
						timer.expires_after(std::chrono::milliseconds(100));
						timer.async_wait([self](const boost::system::error_code&) {
							self->start();
						});
						return;
					}

					// 处理新连接
					std::println("接受新连接: {}", 
						connection->getSocket().remote_endpoint().address().to_string());
					connection->start();
					
					// 继续监听新连接
					self->start();
				}
				catch (const std::exception& ex) {
					std::println("处理连接异常: {}", ex.what());
					
					// 继续监听新连接
					self->start();
				}
				catch (...) {
					std::println("处理连接时发生未知异常");
					self->start();
				}
			});
	}
	catch (const std::exception& ex) {
		std::println("服务器启动异常: {}", ex.what());
		
		// 短暂延迟后重试
		boost::asio::steady_timer timer(_pIoContext);
		timer.expires_after(std::chrono::milliseconds(1));
		timer.async_wait([self = shared_from_this()](const boost::system::error_code&) {
			self->start();
		});
	}
}
