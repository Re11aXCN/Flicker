/*************************************************************************************
 *
 * @ Filename	 : FKHttpConnection.cpp
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
#include "FKHttpConnection.h"
#include <print>

#include "FKLogicSystem.h"
#include "FKUtils.h"
FKHttpConnection::FKHttpConnection(boost::asio::ip::tcp::socket socket)
	: _pSocket(std::move(socket))
	, _pBuffer{ 8192 }
	, _pTimeout{ _pSocket.get_executor(), std::chrono::seconds(60) }
{

}

void FKHttpConnection::start()
{
	boost::beast::http::async_read(_pSocket, _pBuffer, _pRequest, [self = shared_from_this()](boost::beast::error_code ec,
		size_t bytes_transferred) {
			try {
				if (ec) {
					std::println("http read err is {}", ec.what());
					return;
				}

				//处理读到的数据
				boost::ignore_unused(bytes_transferred);
				self->_handleRequest();
				self->_checkTimeout();
			}
			catch (std::exception& ex) {
				std::println("exception is {}", ex.what());
			}
		}
	);
}

void FKHttpConnection::_checkTimeout()
{
	_pTimeout.async_wait(
		[self = shared_from_this()](boost::beast::error_code ec)
		{
			if (!ec)
			{
				// Close socket to cancel any outstanding operation.
				self->_pSocket.close(ec);
			}
		});
}

void FKHttpConnection::_writeResponse()
{
	_pResponse.content_length(_pResponse.body().size());
	boost::beast::http::async_write(
		_pSocket,
		_pResponse,
		[self = shared_from_this()](boost::beast::error_code ec, std::size_t)
		{
			// 关闭发送端
			self->_pSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
			self->_pTimeout.cancel();
		});
}

void FKHttpConnection::_handleRequest()
{
	_pResponse.version(_pRequest.version());
	// 设置为短链接
	_pRequest.keep_alive(false);

	bool success = false;
	switch (_pRequest.method())
	{
	case boost::beast::http::verb::get: {
		_pQueryParams = FKUtils::parse_query_params(_pRequest.target(), _pUrl);
		success = FKLogicSystem::getInstance()->callBack(_pUrl, Http::RequestType::GET, shared_from_this());
		break;
	}		  
	case boost::beast::http::verb::post: {
		success = FKLogicSystem::getInstance()->callBack(_pRequest.target(), Http::RequestType::POST, shared_from_this());
		break;
	}
	default:
		break;
	}

	if (!success) {
		_pResponse.result(boost::beast::http::status::not_found);
		_pResponse.set(boost::beast::http::field::content_type, "text/plain");
		boost::beast::ostream(_pResponse.body()) << "url not found\r\n";
		_writeResponse();
		return;
	}

	_pResponse.result(boost::beast::http::status::ok);
	_pResponse.set(boost::beast::http::field::server, "GateServer");
	_writeResponse();
}