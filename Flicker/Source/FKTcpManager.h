/*************************************************************************************
 *
 * @ Filename     : FKTcpManager.h
 * @ Description  : TCP连接管理器，处理与聊天服务器的长连接通信
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
#ifndef FK_TCP_MANAGER_H_
#define FK_TCP_MANAGER_H_

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QQueue>
#include <QMutex>
#include <memory>

#include "FKDef.h"
#include "universal/macros.h"

class FKTcpManager : public QObject, public std::enable_shared_from_this<FKTcpManager>
{
    Q_OBJECT
        SINGLETON_CREATE_SHARED_H(FKTcpManager)

public:
    ~FKTcpManager();

    // 连接管理 - 添加Q_INVOKABLE确保线程安全
    Q_INVOKABLE void connectToServer(const QString& host, uint16_t port);
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void reconnect();

    // 认证
    Q_INVOKABLE void authenticate();

    // 消息发送
    Q_INVOKABLE void sendChatMessage(const QString& message);
    Q_INVOKABLE void sendHeartbeat();
    Q_INVOKABLE void sendCustomMessage(const QJsonObject& message, Flicker::Tcp::MessageType type = Flicker::Tcp::MessageType::CHAT_MESSAGE);

    // 状态查询
    Flicker::Tcp::ConnectionStates getConnectionState() const { return _connectionStates; }
    bool isConnected() const;
    bool isAuthenticated() const;

    // 配置
    void setAuthenticationInfo(const QString& token, const QString& clientDeviceId);
    void setAutoAuthenticate(bool autoAuthenticate);
    void setHeartbeatInterval(int intervalMs);
    void setConnectTimeout(int timeoutMs);
    void setReconnectSettings(int maxAttempts, int baseTimeoutMs);
    void setHeartbeatRetrySettings(int maxRetries);
    void stopHeartbeat();

Q_SIGNALS:
    // 连接状态变化
    void connectionStateChanged(Flicker::Tcp::ConnectionStates state);
    void connected();
    void disconnected();
    void authenticationResult(bool success, const QString& message);

    // 消息接收
    void messageReceived(Flicker::Tcp::MessageType type, const QJsonObject& message);
    void chatMessageReceived(const QString& sender, const QString& message);
    void systemNotificationReceived(const QString& notification);
    void errorMessageReceived(const QString& error);

    // 错误处理
    void connectionError(const QString& error);
    void protocolError(const QString& error);

    // 心跳相关
    void heartbeatReceived();

private Q_SLOTS:
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketError(QAbstractSocket::SocketError error);
    void _onSocketReadyRead();
    void _onHeartbeatTimeout();
    void _onConnectTimeout();
    void _onReconnectTimeout();

    // 新增槽函数用于异步消息处理
    void _emitMessageSignal(Flicker::Tcp::MessageType type, const QJsonObject& message);

    void _trySendNextPacket();
private:
    explicit FKTcpManager(QObject* parent = nullptr);

    // 消息处理
    void _processReceivedData();
    // 处理发送队列的核心方法（必须在锁内调用）
    void _processSendQueue();
    void _handleCompleteMessage(const Flicker::Tcp::MessageHeader& header, const QByteArray& body);
    void _processMessage(Flicker::Tcp::MessageType messageType, const QJsonObject& message);
    void _handleAuthResponse(const QJsonObject& message);
    void _handleHeartbeat(const QJsonObject& message);
    void _handleChatMessage(const QJsonObject& message);
    void _handleSystemNotification(const QJsonObject& message);
    void _handleErrorMessage(const QJsonObject& message);

    // 消息发送
    void _sendMessage(const QByteArray& data, Flicker::Tcp::MessageType type);
    void _sendRawData(const QByteArray& data);

    // 状态管理
    void _setConnectionState(Flicker::Tcp::ConnectionStates states);
    void _addConnectionState(Flicker::Tcp::ConnectionState flag);
    void _removeConnectionState(Flicker::Tcp::ConnectionState flag);
    bool _hasConnectionState(Flicker::Tcp::ConnectionState flag) const;
    Q_INVOKABLE void _startHeartbeat();
    void _stopHeartbeat();
    void _startReconnectTimer();
    void _stopReconnectTimer();

    // 数据包处理
    bool _parseMessageHeader();
    void _resetPacketState();

private:
    // 定时器
    std::unique_ptr<QTimer> _reconnectTimer;
    std::unique_ptr<QTimer> _connectTimeoutTimer;
    std::unique_ptr<QTimer> _heartbeatTimer;

    // 认证信息
    QString _token;
    QString _clientDeviceId;

    // 发送队列
    QMutex _sendMutex;
    QQueue<QByteArray> _sendQueue;
    bool _isSending{ false };

    bool _autoAuthenticate{ false };
    bool _waitingForHeartbeatResponse{ false }; // 是否正在等待心跳响应

    // 数据包解析
    bool _headerReceived{ false };
    uint32_t _expectedBodyLength{ 0 };
    QByteArray _receiveBuffer;
    Flicker::Tcp::MessageHeader _currentHeader;

    // 网络连接
    std::unique_ptr<QTcpSocket> _socket;
    QString _host;
    Flicker::Tcp::ConnectionStates _connectionStates{ Flicker::Tcp::ConnectionState::DISCONNECTED };
    uint16_t _port{ 0 };

    // 协议常量
    static constexpr uint16_t PROTOCOL_VERSION = 1;
    static constexpr uint32_t MESSAGE_MAGIC = 0x464B4348; // "FKCH"
    static constexpr uint64_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB
    static constexpr uint64_t MAX_QUEUE_SIZE = 1000;  // 最大队列大小

    // 配置参数
    int _heartbeatInterval{ 60000 };        // 心跳间隔（毫秒）
    int _connectTimeout{ 10000 };           // 连接超时（毫秒）
    int _reconnectBaseTimeout{ 5000 };      // 重连基础超时（毫秒）
    int _maxReconnectAttempts{ 3 };         // 最大重连次数
    int _currentReconnectAttempts{ 0 };
    int _heartbeatTimeoutCount{ 0 };        // 心跳超时计数器
    int _maxHeartbeatRetries{ 3 };          // 最大心跳重试次数
    int _currentHeartbeatRetries{ 0 };      // 当前心跳重试次数
    uint64_t _lastHeartbeatSent{ 0 };       // 最后心跳发送时间
    uint64_t _lastHeartbeatReceived{ 0 };   // 最后心跳接收时间
    int64_t _heartbeatSequence{ 0 };       // 心跳序列号
};
#endif // !FK_TCP_MANAGER_H_

