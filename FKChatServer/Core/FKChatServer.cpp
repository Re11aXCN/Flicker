#include "FKChatServer.h"
#include "FKTcpConnection.h"
#include "Library/Logger/logger.h"
#include "Flicker/Global/Asio/FKIoContextThreadPool.h"

FKChatServer::FKChatServer(boost::asio::io_context& ioc,
    const std::string& address,
    uint16_t port,
    const std::string& serverId)
    : _pIpIoContext(ioc)
    , _pAcceptor(ioc)
    , _pAddress(address)
    , _pPort(port)
    , _pServerId(serverId)
    , _pIsRunning(false)
{
    LOGGER_DEBUG("FKChatServer created");
}

void FKChatServer::start()
{
    if (_pIsRunning.load()) {
        LOGGER_WARN("聊天服务器已经在运行中");
        return;
    }

    try {
        // 绑定端口
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(_pAddress), _pPort);
        _pAcceptor.open(endpoint.protocol());
        _pAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        _pAcceptor.bind(endpoint);
        _pAcceptor.listen();

        _pIsRunning.store(true);

        LOGGER_INFO(std::format("聊天服务器启动成功，监听地址: {}:{}", _pAddress, _pPort));

        auto self = shared_from_this();
        _pCleanupTimer = std::make_shared<boost::asio::steady_timer>(_pIpIoContext);
        _pCleanupTimer->expires_after(std::chrono::minutes(5));
        _pCleanupTimer->async_wait([self](const boost::system::error_code& ec) {
            if (!ec && self->_pIsRunning) {
                self->_cleanupExpiredConnections();
                self->_pCleanupTimer->expires_after(std::chrono::minutes(5));
                self->_pCleanupTimer->async_wait(std::bind(&FKChatServer::_cleanupExpiredConnections, self));
            }
            });

        // 开始接受连接
        _acceptConnections();
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("启动聊天服务器失败: {}", e.what()));
        _pIsRunning.store(false);
        throw;
    }
}

void FKChatServer::stop()
{
    if (!_pIsRunning.exchange(false)) {
        return;
    }

    LOGGER_INFO("正在停止聊天服务器...");

    // 关闭acceptor
    boost::system::error_code ec;
    if (_pAcceptor.is_open()) {
        _pAcceptor.close(ec);
        if (ec) {
            LOGGER_ERROR(std::format("关闭acceptor错误: {}", ec.message()));
        }
    }

    // 关闭所有有效连接
    std::vector<std::shared_ptr<FKTcpConnection>> activeConnections;
    {
        std::lock_guard<std::shared_mutex> lock(_pConnectionsMutex);
        for (auto& [userUuid, weak_conn] : _pConnections) {
            if (auto conn = weak_conn.lock()) {
                activeConnections.push_back(conn);
            }
        }
        _pConnections.clear();
    }

    // 在锁外关闭连接
    for (auto& conn : activeConnections) {
        conn->stop();
    }

    LOGGER_INFO(std::format("聊天服务器 {} 停止完成", _pServerId));
}

void FKChatServer::addConnection(const std::string& userUuid, std::shared_ptr<FKTcpConnection> connection)
{
    std::lock_guard<std::shared_mutex> lock(_pConnectionsMutex);

    // 如果用户已经有连接，先关闭旧连接
    auto it = _pConnections.find(userUuid);
    if (it != _pConnections.end()) {
        if (auto old_conn = it->second.lock()) {
            LOGGER_INFO(std::format("用户 {} 已有连接，关闭旧连接", userUuid));
            old_conn->stop();
        }
    }

    _pConnections[userUuid] = connection;
    LOGGER_INFO(std::format("添加用户连接: {}, 当前连接数: {}", userUuid, _pConnections.size()));
}

void FKChatServer::removeConnection(const std::string& userUuid)
{
    std::lock_guard<std::shared_mutex> lock(_pConnectionsMutex);

    auto it = _pConnections.find(userUuid);
    if (it != _pConnections.end()) {
        _pConnections.erase(it);
        LOGGER_INFO(std::format("移除用户连接: {}, 当前连接数: {}", userUuid, _pConnections.size()));
    }
}

std::shared_ptr<FKTcpConnection> FKChatServer::getConnection(const std::string& userUuid)
{
    std::shared_lock<std::shared_mutex> lock(_pConnectionsMutex);

    auto it = _pConnections.find(userUuid);
    if (it != _pConnections.end()) {
        // 尝试提升为 shared_ptr
        if (auto conn = it->second.lock()) {
            return conn;
        }
        else {
            // 连接已失效，标记为需要移除
            LOGGER_DEBUG(std::format("检测到失效连接: {}", userUuid));
        }
    }
    return nullptr;
}

void FKChatServer::broadcastMessage(const std::string& message)
{
    std::vector<std::string> expiredUsers;
    {
        std::shared_lock<std::shared_mutex> lock(_pConnectionsMutex);
        size_t activeConnections = 0;

        for (auto it = _pConnections.begin(); it != _pConnections.end(); ++it) {
            if (auto connection = it->second.lock()) {
                connection->sendMessage(message, Flicker::Tcp::MessageType::CHAT_MESSAGE);
                activeConnections++;
            }
            else {
                // 连接已失效，记录需要移除的用户
                expiredUsers.push_back(it->first);
            }
        }

        LOGGER_INFO(std::format("聊天服务器 {} 广播消息给 {} 个用户",
            _pServerId, activeConnections));
    }

    // 移除失效连接
    if (!expiredUsers.empty()) {
        std::unique_lock<std::shared_mutex> lock(_pConnectionsMutex);
        for (const auto& user : expiredUsers) {
            _pConnections.erase(user);
            LOGGER_DEBUG(std::format("清理失效连接: {}", user));
        }
    }
}

void FKChatServer::sendMessageToUser(const std::string& userUuid, const std::string& message)
{
    auto connection = getConnection(userUuid);
    if (connection) {
        connection->sendMessage(message, Flicker::Tcp::MessageType::CHAT_MESSAGE);
        LOGGER_DEBUG(std::format("发送消息给用户: {}", userUuid));
    }
    else {
        LOGGER_WARN(std::format("用户 {} 不在线，无法发送消息", userUuid));
    }
}

size_t FKChatServer::getConnectionCount() const
{
    std::shared_lock<std::shared_mutex> lock(_pConnectionsMutex);
    return _pConnections.size();
}

int32_t FKChatServer::getCurrentLoad() const
{
    return static_cast<int32_t>(getConnectionCount() * 100 / MAX_CONNECTIONS);
}

void FKChatServer::_acceptConnections()
{
    if (!_pIsRunning.load()) {
        return;
    }
    FKIoContextThreadPool::ioContext& ioc = FKIoContextThreadPool::getInstance()->getNextContext();
    auto self = shared_from_this();
    auto newConnection = std::make_shared<FKTcpConnection>(_pIpIoContext, self);

    // 异步接受连接
    _pAcceptor.async_accept(
        newConnection->getSocket(),
        [self, newConnection](boost::system::error_code ec) {
            self->_handleAccept(newConnection, ec);
        }
    );
}

void FKChatServer::_handleAccept(std::shared_ptr<FKTcpConnection> connection, const boost::system::error_code& ec)
{
    if (!ec) {
        // 检查连接数限制
        if (getConnectionCount() >= MAX_CONNECTIONS) {
            LOGGER_WARN(std::format("聊天服务器 {} 连接数已达上限 {}，拒绝新连接", _pServerId, MAX_CONNECTIONS));
            connection->stop();
        }
        else {
            LOGGER_INFO(std::format("接受新的TCP连接: {}",
            connection->getSocket().remote_endpoint().address().to_string()));

            // 启动连接处理
            connection->start();
        }
    }
    else {
        if (ec == boost::asio::error::operation_aborted) {
            LOGGER_INFO("接受连接操作被取消");
        }
        else {
            LOGGER_ERROR(std::format("接受连接错误: {}", ec.message()));
        }
    }

    // 继续接受下一个连接
    _acceptConnections();
}

void FKChatServer::_cleanupExpiredConnections()
{
    std::vector<std::string> expiredUsers;
    {
        std::shared_lock<std::shared_mutex> lock(_pConnectionsMutex);
        for (const auto& [user, weak_conn] : _pConnections) {
            if (weak_conn.expired()) {
                expiredUsers.push_back(user);
            }
        }
    }

    if (!expiredUsers.empty()) {
        std::unique_lock<std::shared_mutex> lock(_pConnectionsMutex);
        for (const auto& user : expiredUsers) {
            _pConnections.erase(user);
            LOGGER_DEBUG(std::format("定期清理失效连接: {}", user));
        }
    }
}
