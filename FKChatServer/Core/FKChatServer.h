/*************************************************************************************
 *
 * @ Filename     : FKChatServer.h
 * @ Description  : 聊天服务器核心类，负责处理客户端连接、消息转发和用户管理
 *
 * @ Version      : V1.0
 * @ Author       : Re11a
 * @ Date Created : 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By:
 * Modifications:
 * ======================================
*************************************************************************************/
#ifndef FK_CHAT_SERVER_H_
#define FK_CHAT_SERVER_H_

#include <memory>
#include <atomic>
#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "Flicker/Global/FKConfig.h"
#include "Flicker/Global/FKDef.h"
#include "Flicker/Global/universal/macros.h"

class FKTcpConnection;

class FKChatServer : public std::enable_shared_from_this<FKChatServer> {
public:
    explicit FKChatServer(boost::asio::io_context& ioc,
        const std::string& address,
        uint16_t port,
        const std::string& serverId);
    ~FKChatServer() = default;

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 获取服务器状态
    bool isRunning() const { return _pIsRunning.load(); }

    // 获取当前连接数
    size_t getConnectionCount() const;

    // 获取服务器负载百分比
    int32_t getCurrentLoad() const;

    // 获取服务器ID
    const std::string& getServerId() const { return _pServerId; }

    // 连接管理
    void addConnection(const std::string& userUuid, std::shared_ptr<FKTcpConnection> connection);
    void removeConnection(const std::string& userUuid);
    std::shared_ptr<FKTcpConnection> getConnection(const std::string& userUuid);

    // 消息转发
    void broadcastMessage(const std::string& message);
    void sendMessageToUser(const std::string& userUuid, const std::string& message);

private:
    void _acceptConnections();
    void _handleAccept(std::shared_ptr<FKTcpConnection> connection, const boost::system::error_code& ec);
    void _cleanupExpiredConnections();
private:
    boost::asio::io_context& _pIpIoContext;
    boost::asio::ip::tcp::acceptor _pAcceptor;

    std::string _pAddress;
    std::string _pServerId;
    uint16_t _pPort;

    std::atomic<bool> _pIsRunning{ false };

    // 连接管理
    std::shared_ptr<boost::asio::steady_timer> _pCleanupTimer{ nullptr };
    mutable std::shared_mutex _pConnectionsMutex;
    std::unordered_map<std::string, std::weak_ptr<FKTcpConnection>> _pConnections;

    // 配置
    static constexpr size_t MAX_CONNECTIONS = 10000;
};

#endif // FK_CHAT_SERVER_H_