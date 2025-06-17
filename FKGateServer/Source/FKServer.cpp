/*************************************************************************************
 *
 * @ Filename	 : FKServer.cpp
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#include "FKServer.h"
#include "FKHttpConnection.h"

#include <print>
FKServer::FKServer(boost::asio::io_context& ioc, UINT16 port)
	: _pIoContext(ioc)
	, _pAcceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	, _pSocket(ioc)
{

}

void FKServer::start()
{
	_pAcceptor.async_accept(_pSocket, [self = shared_from_this()](boost::beast::error_code ec) {
		try {
			// 出错则放弃这个连接，继续监听新链接
			if (ec) {
				self->start();
				return;
			}

			// 处理新链接，创建HpptConnection类管理新连接
			std::make_shared<FKHttpConnection>(std::move(self->_pSocket))->start();
			// 继续监听
			self->start();
		}
		catch (std::exception& ex) {
			std::println("exception is {}" ,ex.what());
			self->start();
		}
		});
}
