/*************************************************************************************
 *
 * @ Filename     : FKHttpConnection.h
 * @ Description : 建立和 Flicker客户端 的链接 将HTTP请求(如GET/POST等)转发给LogicSystem处理，
 *                   并将处理结果发送给 Flicker客户端
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
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
#ifndef FK_HTTP_CONNECTION_H_
#define FK_HTTP_CONNECTION_H_

#include <string>
#include <unordered_map>
#include <atomic>
#include <functional>

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

class FKHttpConnection : public std::enable_shared_from_this<FKHttpConnection>
{
public:
    // 定义关闭回调函数类型
    using CloseCallback = std::function<void()>;
    
    explicit FKHttpConnection(boost::asio::io_context& ioc);
    ~FKHttpConnection();
    void start();
    void stop();
    boost::asio::ip::tcp::socket& getSocket() { return _pSocket; };
    boost::beast::http::request<boost::beast::http::dynamic_body>& getRequest() { return _pRequest; };
    boost::beast::http::response<boost::beast::http::dynamic_body>& getResponse() { return _pResponse; };
    std::unordered_map<std::string, std::string> getQueryParams() const { return _pQueryParams; };
    
    // 设置关闭回调函数
    void setCloseCallback(CloseCallback callback) { _pCloseCallback = std::move(callback); };
private:
    void _checkTimeout();
    void _writeResponse();
    void _handleRequest();
    void _closeConnection();

    boost::asio::ip::tcp::socket _pSocket;
    boost::beast::flat_buffer _pBuffer;
    boost::beast::http::request<boost::beast::http::dynamic_body> _pRequest;
    boost::beast::http::response<boost::beast::http::dynamic_body> _pResponse;
    boost::asio::steady_timer _pTimeout;

    std::string _pUrl;
    std::unordered_map<std::string, std::string> _pQueryParams;
    
    // 连接状态
    std::atomic<bool> _pIsClosed{false};
    
    // 关闭回调函数
    CloseCallback _pCloseCallback;
};

#endif // !FK_HTTP_CONNECTION_H_


