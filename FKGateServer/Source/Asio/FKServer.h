﻿/*************************************************************************************
 *
 * @ Filename     : FKServer.h
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author      : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_SERVER_H_
#define FK_SERVER_H_

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

class FKServer : public std::enable_shared_from_this<FKServer>
{
public:
    FKServer(boost::asio::io_context& ioc, UINT16 port);
    ~FKServer() = default;
    void start();
    void stop();
    bool isRunning() const { return _pIsRunning; }
private:
    boost::asio::io_context& _pIoContext;
    boost::asio::ip::tcp::acceptor _pAcceptor;
    std::atomic<bool> _pIsRunning{false};
    std::atomic<size_t> _pActiveConnections{0};
    
    // 增加或减少活动连接计数
    void _incrementActiveConnections();
    void _decrementActiveConnections();
};

#endif // !FK_SERVER_H_

