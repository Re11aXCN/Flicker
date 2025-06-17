/*************************************************************************************
 *
 * @ Filename	 : FKHttpConnection.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V1.0          Modify Time: 2025/6/17        Modified By: Re11a
 * Modifications: 
 * @Member _pSocket
 * @Member _pBuffer: 用来存储接收到的HTTP请求报文
 * @Member _pRequest: 用来解析HTTP请求报文
 * @Member _pResponse: 用来响应客户端的HTTP请求
 * @Member _pTimeout: 用来设置HTTP请求的超时时间
 * ======================================
*************************************************************************************/
#ifndef FK_HTTPCONNECTION_H_
#define FK_HTTPCONNECTION_H_

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include "FKMarco.h"
class FKHttpConnection : public std::enable_shared_from_this<FKHttpConnection>
{
public:
	explicit FKHttpConnection(boost::asio::ip::tcp::socket socket);
	~FKHttpConnection() = default;
	void start();
	boost::beast::http::response<boost::beast::http::dynamic_body>& getResponse() { return _pResponse; };
private:
	void _checkTimeout();
	void _writeResponse();
	void _handleRequest();

	boost::asio::ip::tcp::socket _pSocket;
	boost::beast::flat_buffer _pBuffer;
	boost::beast::http::request<boost::beast::http::dynamic_body> _pRequest;
	boost::beast::http::response<boost::beast::http::dynamic_body> _pResponse;
	boost::asio::steady_timer _pTimeout;
};

#endif // !FK_HTTPCONNECTION_H_


