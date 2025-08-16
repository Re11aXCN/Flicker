#ifndef FK_TCP_CONNECTION_H_
#define FK_TCP_CONNECTION_H_

#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <string>
#include <functional>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "Flicker/Global/FKDef.h"

class FKChatServer;

class FKTcpConnection : public std::enable_shared_from_this<FKTcpConnection> {
public:
    // 定义关闭回调函数类型
    using CloseCallback = std::function<void(const std::string& userUuid)>;

    explicit FKTcpConnection(boost::asio::io_context& ioc, std::shared_ptr<FKChatServer> server);
    ~FKTcpConnection();

    // 启动连接处理
    void start();

    // 停止连接
    void stop();

    // 获取socket引用
    boost::asio::ip::tcp::socket& getSocket() { return _pSocket; }

    // 发送消息
    void sendMessage(const std::string& message, Flicker::Tcp::MessageType type);

    // 获取用户UUID
    const std::string& getUserUuid() const { return _pUserUuid; }

    // 获取客户端设备ID
    const std::string& getClientDeviceId() const { return _pClientDeviceId; }

    // 检查是否已认证
    bool isAuthenticated() const { return _pIsAuthenticated.load(); }

    // 设置关闭回调函数
    void setCloseCallback(CloseCallback callback) { _pCloseCallback = std::move(callback); }

private:
    // 消息处理
    void _readMessage();
    void _processReceivedData();
    void _handleCompleteMessage(const Flicker::Tcp::MessageHeader& header, const std::string& body);
    void _processMessage(Flicker::Tcp::MessageType messageType, const std::string& messageBody);

    // 具体消息处理方法
    void _handleAuthRequest(const std::string& messageBody);
    void _handleHeartbeat(const std::string& messageBody);
    void _handleChatMessage(const std::string& messageBody);

    // 消息发送
    void _sendMessage(const std::string& data, Flicker::Tcp::MessageType type);
    void _writeMessage();

    // 数据包解析
    bool _parseMessageHeader();
    void _resetPacketState();

    // 连接管理
    void _closeConnection();
    void _checkTimeout();

    // Token验证
    bool _validateToken(const std::string& token, const std::string& clientDeviceId);

    // 发送响应消息
    void _sendAuthResponse(bool success, const std::string& message);
    void _sendHeartbeatResponse();
    void _sendErrorMessage(const std::string& error);

private:
    boost::asio::io_context& _pIoContext;
    boost::asio::ip::tcp::socket _pSocket;
    boost::asio::steady_timer _pTimeout;
    std::weak_ptr<FKChatServer> _pServer;

    // 用户信息
    std::string _pUserUuid;
    std::string _pClientDeviceId;

    // 数据接收缓冲区
    std::vector<uint8_t> _pReceiveBuffer;

    // 发送队列
    std::queue<std::vector<uint8_t>> _pSendQueue;
    std::mutex _pSendMutex;
    bool _pIsSending{ false };

    // 连接状态
    std::atomic<bool> _pIsClosed{ false };
    std::atomic<bool> _pIsAuthenticated{ false };

    // 数据包解析状态
    bool _pHeaderReceived{ false };
    uint32_t _pExpectedBodyLength{ 0 };
    size_t _pReceiveProcessed{ 0 };        // 已处理数据的偏移量
    Flicker::Tcp::MessageHeader _pCurrentHeader;

    // 关闭回调函数
    CloseCallback _pCloseCallback;

    // 协议常量
    static constexpr uint16_t PROTOCOL_VERSION = 1;
    static constexpr uint32_t MESSAGE_MAGIC = 0x464B4348; // "FKCH"
    static constexpr uint32_t MIN_BUFFER_SIZE = 1024; // 最小缓冲区大小 1KB
    static constexpr uint32_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB
    static constexpr size_t MAX_WRITE_QUEUE = 100; // 最大待发消息数
    static constexpr std::chrono::seconds AUTH_TIMEOUT{ 8 }; // 认证超时时间
    static constexpr std::chrono::seconds HEARTBEAT_TIMEOUT{ 90 }; // 心跳超时时间，比客户端心跳间隔(60s)长
};

#endif // FK_TCP_CONNECTION_H_