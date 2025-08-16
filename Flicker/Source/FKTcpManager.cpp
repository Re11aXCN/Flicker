#include "FKTcpManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QThread>
#include <QThreadPool>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QUuid>

#include "universal/utils.h"
#include "Library/Logger/logger.h"

SINGLETON_CREATE_SHARED_CPP(FKTcpManager)

using namespace Flicker::Tcp;
FKTcpManager::FKTcpManager(QObject* parent)
    : QObject(parent)
    , _socket(std::make_unique<QTcpSocket>(this))
    , _reconnectTimer(std::make_unique<QTimer>(this))
    , _connectTimeoutTimer(std::make_unique<QTimer>(this))
    , _heartbeatTimer(std::make_unique<QTimer>(this))
{
    LOGGER_DEBUG("FKTcpManager constructed");

    // 配置Socket
    _socket->setProxy(QNetworkProxy::NoProxy);

    // 设置定时器为单次触发
    _heartbeatTimer->setSingleShot(false);
    _connectTimeoutTimer->setSingleShot(true);
    _reconnectTimer->setSingleShot(true);

    // 连接Socket信号
    QObject::connect(_socket.get(), &QTcpSocket::connected, this, &FKTcpManager::_onSocketConnected);
    QObject::connect(_socket.get(), &QTcpSocket::disconnected, this, &FKTcpManager::_onSocketDisconnected);
    QObject::connect(_socket.get(), QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
        this, &FKTcpManager::_onSocketError);
    QObject::connect(_socket.get(), &QTcpSocket::readyRead, this, &FKTcpManager::_onSocketReadyRead);
    QObject::connect(_socket.get(), &QTcpSocket::bytesWritten, this, &FKTcpManager::_trySendNextPacket);

    // 连接定时器信号
    QObject::connect(_reconnectTimer.get(), &QTimer::timeout, this, &FKTcpManager::_onReconnectTimeout);
    QObject::connect(_connectTimeoutTimer.get(), &QTimer::timeout, this, &FKTcpManager::_onConnectTimeout);
    QObject::connect(_heartbeatTimer.get(), &QTimer::timeout, this, &FKTcpManager::_onHeartbeatTimeout);
}

FKTcpManager::~FKTcpManager()
{
    LOGGER_DEBUG("FKTcpManager destructing");

    // 先停止所有定时器
    _reconnectTimer->stop();
    _connectTimeoutTimer->stop();
    _heartbeatTimer->stop();

    // 清理发送队列
    {
        QMutexLocker locker(&_sendMutex);
        _sendQueue.clear();
        _isSending = false;
    }

    // 断开socket连接
    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        _socket->disconnectFromHost();
        _socket->waitForDisconnected(1000);
    }

    LOGGER_DEBUG("FKTcpManager destructed");
}

// ==================== 连接管理 ====================

void FKTcpManager::connectToServer(const QString& host, uint16_t port)
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "connectToServer", Qt::QueuedConnection,
            Q_ARG(QString, host), Q_ARG(uint16_t, port));
        return;
    }

    if (_connectionStates.testFlag(ConnectionState::CONNECTING) ||
        _connectionStates.testFlag(ConnectionState::CONNECTED)) {
        LOGGER_WARN("Already connecting or connected to server");
        return;
    }

    LOGGER_INFO(std::format("Connecting to server: {}:{}", host.toStdString(), port));

    _host = host;
    _port = port;

    // 如果已经连接，先断开
    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        LOGGER_WARN("Socket already connected, disconnecting first");
        disconnectFromServer();
        return;
    }

    // 设置连接状态
    _setConnectionState(ConnectionStates(ConnectionState::CONNECTING));

    // 启动连接超时定时器
    if (_connectTimeout > 0) {
        _connectTimeoutTimer->start(_connectTimeout * 1000);
    }

    // 开始TCP连接
    _socket->connectToHost(host, port);
}

void FKTcpManager::disconnectFromServer()
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "disconnectFromServer", Qt::QueuedConnection);
        return;
    }

    LOGGER_INFO("Disconnecting from server");

    // 停止所有定时器
    _stopHeartbeat();
    _stopReconnectTimer();
    _connectTimeoutTimer->stop();

    // 重置重连计数
    _currentReconnectAttempts = 0;

    // 清理发送队列
    {
        QMutexLocker locker(&_sendMutex);
        _sendQueue.clear();
        _isSending = false;
    }

    // 清理接收缓冲区
    _receiveBuffer.clear();
    _resetPacketState();

    // 断开socket连接
    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        _socket->disconnectFromHost();
        if (_socket->state() != QAbstractSocket::UnconnectedState) {
            _socket->waitForDisconnected(3000);
        }
    }
}

void FKTcpManager::reconnect()
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "reconnect", Qt::QueuedConnection);
        return;
    }

    LOGGER_INFO("Manual reconnect requested");

    // 如果已经连接，先断开
    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        disconnectFromServer();
    }

    // 如果有主机和端口信息，立即重连
    if (!_host.isEmpty() && _port > 0) {
        connectToServer(_host, _port);
    }
    else {
        LOGGER_WARN("No host/port information for reconnection");
    }
}

// ==================== 认证 ====================

void FKTcpManager::setAuthenticationInfo(const QString& token, const QString& clientDeviceId)
{
    _token = token;
    _clientDeviceId = clientDeviceId;
}

void FKTcpManager::setConnectTimeout(int timeoutMs)
{
    _connectTimeout = timeoutMs;
    LOGGER_DEBUG(std::format("Connect timeout set to {} ms", timeoutMs));
}
void FKTcpManager::setReconnectSettings(int maxAttempts, int baseTimeoutMs)
{
    _maxReconnectAttempts = maxAttempts;
    _reconnectBaseTimeout = baseTimeoutMs;
    LOGGER_DEBUG(std::format("Reconnect settings: max attempts = {}, base timeout = {} ms",
        maxAttempts, baseTimeoutMs));
}

void FKTcpManager::setHeartbeatRetrySettings(int maxRetries)
{
    _maxHeartbeatRetries = maxRetries;
    LOGGER_DEBUG(std::format("Heartbeat retry settings: max retries = {}", maxRetries));
}

void FKTcpManager::setAutoAuthenticate(bool autoAuthenticate)
{
    _autoAuthenticate = autoAuthenticate;
}

void FKTcpManager::setHeartbeatInterval(int intervalMs)
{
    _heartbeatInterval = intervalMs;
    LOGGER_DEBUG(std::format("Heartbeat interval set to {} ms", intervalMs));

    // 如果心跳定时器正在运行，重新启动以应用新间隔
    if (_heartbeatTimer->isActive()) {
        _stopHeartbeat();
        _startHeartbeat();
    }
}

void FKTcpManager::stopHeartbeat()
{
    LOGGER_INFO("Manually stopping heartbeat");
    _stopHeartbeat();
}

bool FKTcpManager::isConnected() const
{
    return _connectionStates.testFlag(ConnectionState::CONNECTED);
}

bool FKTcpManager::isAuthenticated() const
{
    return _connectionStates.testFlag(ConnectionState::AUTHENTICATED);
}

void FKTcpManager::authenticate()
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "authenticate", Qt::QueuedConnection);
        return;
    }

    if (!isConnected()) {
        LOGGER_WARN("Cannot authenticate: not connected to server");
        return;
    }

    if (_token.isEmpty() || _clientDeviceId.isEmpty()) {
        LOGGER_ERROR("Cannot authenticate: missing token or client device ID");
        Q_EMIT authenticationResult(false, "Missing authentication credentials");
        return;
    }

    LOGGER_INFO("Starting authentication process");

    QJsonObject authMessage;
    authMessage["token"] = _token;
    authMessage["client_device_id"] = _clientDeviceId;
    authMessage["client_version"] = "flicker_client_v1.0";
    authMessage["client_platform"] = universal::utils::qstring::qconcat(
        QSysInfo::prettyProductName().toUpper(), "_",
        QSysInfo::currentCpuArchitecture(), "_",
        QSysInfo::kernelType(), "_",
        QSysInfo::kernelVersion());

    // 设置认证状态，必须在已连接的基础上
    if (_connectionStates.testFlag(ConnectionState::CONNECTED)) {
        _addConnectionState(ConnectionState::AUTHENTICATING);
    }
    else {
        LOGGER_WARN("Cannot authenticate: not connected to server");
        return;
    }

    QJsonDocument doc(authMessage);
    _sendMessage(doc.toJson(QJsonDocument::Compact), MessageType::AUTH_REQUEST);
}

// ==================== 消息发送 ====================

void FKTcpManager::sendChatMessage(const QString& message)
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "sendChatMessage", Qt::QueuedConnection,
            Q_ARG(QString, message));
        return;
    }

    if (!isConnected()) {
        LOGGER_WARN("Cannot send message: not connected");
        return;
    }

    if (!isAuthenticated()) {
        LOGGER_WARN("Cannot send chat message: not authenticated");
        return;
    }

    if (message.isEmpty()) {
        LOGGER_WARN("Cannot send empty chat message");
        return;
    }

    LOGGER_DEBUG(std::format("Sending chat message: {}", message.toStdString()));

    QJsonObject chatMessage;
    chatMessage["content"] = message;
    chatMessage["timestamp"] = QDateTime::currentSecsSinceEpoch();
    chatMessage["message_id"] = QUuid::createUuid().toString();
    chatMessage["sender"] = _clientDeviceId; // 添加发送者信息

    QJsonDocument doc(chatMessage);
    _sendMessage(doc.toJson(QJsonDocument::Compact), MessageType::CHAT_MESSAGE);
}

void FKTcpManager::sendHeartbeat()
{
    // 确保在对象线程中执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "sendHeartbeat", Qt::QueuedConnection);
        return;
    }

    if (!isConnected()) {
        LOGGER_TRACE("Cannot send heartbeat: not connected");
        return;
    }

    if (!isAuthenticated()) {
        LOGGER_TRACE("Cannot send heartbeat: not authenticated");
        return;
    }

    LOGGER_TRACE("Sending heartbeat");

    QJsonObject heartbeatMessage;
    heartbeatMessage["timestamp"] = QDateTime::currentSecsSinceEpoch();
    heartbeatMessage["client_status"] = "active";
    heartbeatMessage["client_device_id"] = _clientDeviceId;
    heartbeatMessage["sequence"] = ++_heartbeatSequence;

    QJsonDocument doc(heartbeatMessage);
    _sendMessage(doc.toJson(QJsonDocument::Compact), MessageType::HEARTBEAT);

    // 记录心跳发送时间并设置等待响应标志
    _lastHeartbeatSent = QDateTime::currentMSecsSinceEpoch();
    _waitingForHeartbeatResponse = true;
}

void FKTcpManager::sendCustomMessage(const QJsonObject& message, MessageType type)
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "sendCustomMessage", Qt::QueuedConnection,
            Q_ARG(QJsonObject, message), Q_ARG(MessageType, type));
        return;
    }

    if (!isConnected()) {
        LOGGER_WARN("Cannot send custom message: not connected");
        return;
    }

    // 对于某些消息类型，不需要认证
    if (type != MessageType::AUTH_REQUEST && !isAuthenticated()) {
        LOGGER_WARN("Cannot send custom message: not authenticated");
        return;
    }

    if (message.isEmpty()) {
        LOGGER_WARN("Cannot send empty custom message");
        return;
    }

    LOGGER_DEBUG(std::format("Sending custom message of type: {}",
        magic_enum::enum_name(type)));

    QJsonDocument doc(message);
    _sendMessage(doc.toJson(QJsonDocument::Compact), type);
}

// ==================== Socket事件处理 ====================

void FKTcpManager::_onSocketConnected()
{
    LOGGER_INFO("Socket connected to server");

    // 停止连接超时定时器
    _connectTimeoutTimer->stop();

    // 重置重连计数
    _currentReconnectAttempts = 0;

    // 设置连接状态
    _setConnectionState(ConnectionStates(ConnectionState::CONNECTED));

    Q_EMIT connected();

    // 如果有认证信息，自动开始认证
    if (_autoAuthenticate && !_token.isEmpty() && !_clientDeviceId.isEmpty()) {
        authenticate();
    }
}

void FKTcpManager::_onSocketDisconnected()
{
    //LOGGER_INFO("Socket disconnected from server");
    //
    //// 停止所有定时器
    //_stopHeartbeat();
    //_connectTimeoutTimer->stop();
    //
    //// 保存旧状态
    //ConnectionStates oldState = _connectionStates;

    // 设置断开连接状态
    if (_connectionStates.testFlag(ConnectionState::ERROR_STATE)) {
        _addConnectionState(ConnectionState::DISCONNECTED);
    }
    else {
        _setConnectionState(ConnectionStates(ConnectionState::DISCONNECTED));
    }

    //// 清理缓冲区和状态
    //_receiveBuffer.clear();
    //_resetPacketState();
    //
    //// 清理发送队列
    //{
    //    QMutexLocker locker(&_sendMutex);
    //    _sendQueue.clear();
    //    _isSending = false;
    //}

    Q_EMIT disconnected();

    // 如果之前是连接状态且启用了重连，则开始重连
    //if ((oldState.testFlag(ConnectionState::CONNECTED) || oldState.testFlag(ConnectionState::AUTHENTICATED)) &&
    //    _maxReconnectAttempts > 0 && _currentReconnectAttempts < _maxReconnectAttempts) {
    //    _startReconnectTimer();
    //}
    //else if (_currentReconnectAttempts >= _maxReconnectAttempts) {
    //    LOGGER_WARN(std::format("Max reconnect attempts ({}) reached", _maxReconnectAttempts));
    //    Q_EMIT connectionError("Max reconnect attempts reached");
    //}
}

void FKTcpManager::_onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = _socket->errorString();
    LOGGER_ERROR(std::format("Socket error: {} - {}",
        magic_enum::enum_name(error), errorString.toStdString()));

    // 停止连接超时定时器
    _connectTimeoutTimer->stop();

    // 设置错误状态
    _setConnectionState(ConnectionStates(ConnectionState::ERROR_STATE));

    Q_EMIT connectionError(universal::utils::qstring::qconcat(errorString, "!Try connecting % 1 of % 2  times").arg(_currentReconnectAttempts).arg(_maxReconnectAttempts));

    // 根据错误类型决定是否重连
    bool shouldReconnect = false;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::HostNotFoundError:
    case QAbstractSocket::NetworkError:
        shouldReconnect = true;
        break;
    case QAbstractSocket::SocketTimeoutError:
        shouldReconnect = true;
        break;
    default:
        // 其他错误类型不自动重连
        break;
    }

    if (shouldReconnect && _maxReconnectAttempts > 0 &&
        _currentReconnectAttempts < _maxReconnectAttempts) {
        LOGGER_INFO("Will attempt to reconnect due to recoverable error");
        _startReconnectTimer();
    }
}

void FKTcpManager::_onSocketReadyRead()
{
    // 一次性读取所有可用数据（Qt内部有缓冲区管理）
    QByteArray newData = _socket->readAll();
    if (!newData.isEmpty()) {
        LOGGER_TRACE(std::format("Received {} bytes", newData.size()));
        _receiveBuffer.append(newData);
        _processReceivedData();
    }
}

void FKTcpManager::_onHeartbeatTimeout()
{
    // 检查是否正在等待心跳响应
    if (_waitingForHeartbeatResponse) {
        // 服务器没有响应上一次心跳
        _currentHeartbeatRetries++;
        LOGGER_WARN(std::format("Heartbeat timeout, retry {}/{}",
            _currentHeartbeatRetries, _maxHeartbeatRetries));

        if (_currentHeartbeatRetries >= _maxHeartbeatRetries) {
            // 达到最大重试次数，主动断开连接
            LOGGER_ERROR("Server not responding to heartbeat, disconnecting");
            _stopHeartbeat();
            disconnectFromServer();
            Q_EMIT connectionError("Server not responding to heartbeat");
            return;
        }
    }

    // 发送心跳
    sendHeartbeat();
}

void FKTcpManager::_onConnectTimeout()
{
    LOGGER_WARN("Connection timeout occurred");

    // 停止连接超时定时器
    _connectTimeoutTimer->stop();

    // 断开当前连接尝试
    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        _socket->abort();
    }

    // 设置错误状态
    _setConnectionState(ConnectionStates(ConnectionState::ERROR_STATE));

    Q_EMIT connectionError(QString("Connection timeout! Try connecting %1 of %2  times").arg(_currentReconnectAttempts).arg(_maxReconnectAttempts));

    // 如果启用重连，开始重连
    if (_maxReconnectAttempts > 0 && _currentReconnectAttempts < _maxReconnectAttempts) {
        _startReconnectTimer();
    }
}

void FKTcpManager::_onReconnectTimeout()
{
    if (_currentReconnectAttempts < _maxReconnectAttempts) {
        _currentReconnectAttempts++;
        LOGGER_INFO(std::format("Reconnect attempt {} of {}",
            _currentReconnectAttempts, _maxReconnectAttempts));
        connectToServer(_host, _port);
    }
    else {
        LOGGER_ERROR("Max reconnect attempts reached");
        Q_EMIT connectionError("Max reconnect attempts reached! Cannot connect to server!");
        this->disconnectFromServer();
    }
}


// ==================== 消息处理 ====================

void FKTcpManager::_processReceivedData()
{
    while (true) {
        // 如果还没有接收到完整的消息头
        if (!_headerReceived) {
            if (_receiveBuffer.size() < sizeof(MessageHeader)) {
                break; // 数据不够，等待更多数据
            }

            if (!_parseMessageHeader()) {
                LOGGER_ERROR("Failed to parse message header");
                // 增加错误处理
                _receiveBuffer.clear();
                _resetPacketState();
                disconnectFromServer();
                Q_EMIT protocolError("Invalid message header");
                break;
            }
        }

        // 检查是否接收到完整的消息体
        if (_receiveBuffer.size() < (qsizetype)(sizeof(MessageHeader) + _expectedBodyLength)) {
            break; // 数据不够，等待更多数据
        }

        // 提取消息体
        QByteArray messageBody = _receiveBuffer.mid(sizeof(MessageHeader), _expectedBodyLength);
        _receiveBuffer.remove(0, sizeof(MessageHeader) + _expectedBodyLength);

        // 处理完整消息
        _handleCompleteMessage(_currentHeader, messageBody);

        // 重置状态，准备处理下一个消息
        _resetPacketState();
    }
}

void FKTcpManager::_processSendQueue()
{
    while (!_sendQueue.isEmpty()) {
        // 查看队列头部的包，但不取出
        QByteArray& data = _sendQueue.head();

        // 检查socket状态
        if (_socket->state() != QAbstractSocket::ConnectedState) {
            LOGGER_WARN("Socket not connected, clearing send queue");
            _sendQueue.clear();
            _isSending = false;
            return;
        }

        qint64 bytesWritten = _socket->write(data);

        if (bytesWritten <= 0) {
            if (bytesWritten == -1) {
                LOGGER_ERROR(std::format("Failed to write data to socket: {}", _socket->errorString().toStdString()));
            }
            _isSending = false;
            return;
        }

        if (bytesWritten < data.size()) {
            // 部分写入，更新队列中的包
            data = data.mid(bytesWritten);
            // 保持_isSending为true，等待下次发送机会
            return;
        }

        // 完整写入，移除包
        _sendQueue.dequeue();
    }

    // 所有数据发送完成，重置状态
    _isSending = false;
}

bool FKTcpManager::_parseMessageHeader()
{
    if (_receiveBuffer.size() < sizeof(MessageHeader)) {
        return false;
    }

    std::memcpy(&_currentHeader, _receiveBuffer.constData(), sizeof(MessageHeader));

    // 验证魔数
    if (_currentHeader.magic != MESSAGE_MAGIC) {
        LOGGER_ERROR("Invalid magic number");
        return false;
    }

    // 验证消息长度
    if (_currentHeader.length > MAX_MESSAGE_SIZE) {
        LOGGER_ERROR("Message too large");
        return false;
    }

    // 验证协议版本
    if (_currentHeader.version != PROTOCOL_VERSION) {
        LOGGER_ERROR("Unsupported protocol version");
        return false;
    }

    _expectedBodyLength = _currentHeader.length;
    _headerReceived = true;
    return true;
}

void FKTcpManager::_handleCompleteMessage(const MessageHeader& header, const QByteArray& body)
{
    MessageType messageType = static_cast<MessageType>(header.type);

    // 解析JSON消息体
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOGGER_ERROR(std::format("Failed to parse JSON message: {}", parseError.errorString().toStdString()));
        Q_EMIT protocolError("Invalid JSON message");
        return;
    }

    QJsonObject message = doc.object();

    // 使用异步方式处理消息，避免阻塞网络线程
    QThreadPool::globalInstance()->start([this, messageType, message]() {
        // 在线程池中处理业务逻辑
        this->_processMessage(messageType, message);
        });
}

void FKTcpManager::_processMessage(MessageType messageType, const QJsonObject& message)
{
    // 根据消息类型处理
    switch (messageType) {
    case MessageType::AUTH_RESPONSE:
        _handleAuthResponse(message);
        break;
    case MessageType::HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MessageType::CHAT_MESSAGE:
        _handleChatMessage(message);
        break;
    case MessageType::SYSTEM_NOTIFICATION:
        _handleSystemNotification(message);
        break;
    case MessageType::ERROR_MESSAGE:
        _handleErrorMessage(message);
        break;
    default:
        LOGGER_ERROR(std::format("Received unknown message type: {}", magic_enum::enum_name(messageType)));
        break;
    }

    // 发射通用消息信号
    QMetaObject::invokeMethod(this, "_emitMessageSignal", Qt::QueuedConnection,
        Q_ARG(MessageType, messageType),
        Q_ARG(QJsonObject, message));
}

void FKTcpManager::_handleAuthResponse(const QJsonObject& message)
{
    bool success = message["success"].toBool();
    QString responseMessage = message["message"].toString();

    if (success) {
        LOGGER_INFO("Authentication successful");
        _addConnectionState(ConnectionState::AUTHENTICATED);
        _startHeartbeat();
    }
    else {
        LOGGER_WARN(std::format("Authentication failed: {}", responseMessage.toStdString()));
        _removeConnectionState(ConnectionState::AUTHENTICATING);
        // 保持连接状态，但移除认证相关状态
    }

    Q_EMIT authenticationResult(success, responseMessage);
}

void FKTcpManager::_handleHeartbeat(const QJsonObject& message)
{
    // 记录收到心跳响应
    _lastHeartbeatReceived = QDateTime::currentMSecsSinceEpoch();

    // 重置心跳重试计数和等待标志
    _currentHeartbeatRetries = 0;
    _waitingForHeartbeatResponse = false;

    // 检查心跳序列号
    if (message.contains("sequence")) {
        qint64 receivedSequence = message["sequence"].toVariant().toLongLong();
        LOGGER_TRACE(std::format("Received heartbeat response, sequence: {}", receivedSequence));

        // 可以在这里验证序列号是否匹配
        if (receivedSequence != _heartbeatSequence) {
            LOGGER_WARN(std::format("Heartbeat sequence mismatch: expected {}, got {}",
                _heartbeatSequence, receivedSequence));
        }
    }
    else {
        LOGGER_TRACE("Received heartbeat response");
    }

    // 发出心跳响应信号
    Q_EMIT heartbeatReceived();
}

void FKTcpManager::_handleChatMessage(const QJsonObject& message)
{
    QString sender = message["sender"].toString();
    QString content = message["content"].toString();

    Q_EMIT chatMessageReceived(sender, content);
}

void FKTcpManager::_handleSystemNotification(const QJsonObject& message)
{
    QString notification = message["content"].toString();

    Q_EMIT systemNotificationReceived(notification);
}

void FKTcpManager::_handleErrorMessage(const QJsonObject& message)
{
    QString error = message["error"].toString();

    Q_EMIT errorMessageReceived(error);
}

// ==================== 消息发送实现 ====================

void FKTcpManager::_sendMessage(const QByteArray& data, MessageType type)
{
    // 构建消息头
    MessageHeader header;
    header.timestamp = QDateTime::currentSecsSinceEpoch();
    header.magic = MESSAGE_MAGIC;
    header.length = data.size();
    header.type = static_cast<uint16_t>(type);
    header.version = PROTOCOL_VERSION;
    header.reserved = 0;

    // 组装完整消息
    QByteArray fullMessage;
    fullMessage.append(reinterpret_cast<const char*>(&header), sizeof(header));
    fullMessage.append(data);

    _sendRawData(fullMessage);
}

void FKTcpManager::_sendRawData(const QByteArray& data)
{
    QMutexLocker locker(&_sendMutex);

    // 高负载时限制队列大小
    if (_sendQueue.size() > MAX_QUEUE_SIZE) {
        LOGGER_WARN("Send queue full, discarding message");
        return;
    }

    // 将数据加入队列
    _sendQueue.enqueue(data);

    // 如果当前没有发送任务，立即发送
    if (!_isSending) {
        _isSending = true;
        _processSendQueue();  // 直接处理发送队列
    }
}

void FKTcpManager::_trySendNextPacket()
{
    // 只有在有数据需要发送时才处理
    if (_isSending) {
        QMutexLocker locker(&_sendMutex);
        _processSendQueue();
    }
}

// ==================== 状态管理 ====================

void FKTcpManager::_setConnectionState(ConnectionStates states)
{
    ConnectionStates oldStates = _connectionStates;
    _connectionStates = states;

    if (oldStates != _connectionStates) {
        // 特殊状态转换处理
        if (oldStates.testFlag(ConnectionState::AUTHENTICATED) &&
            !_connectionStates.testFlag(ConnectionState::AUTHENTICATED)) {
            // 从认证状态变为非认证状态时停止心跳
            _stopHeartbeat();
        }

        // 连接断开或错误时清空发送队列和重置心跳重试
        if (_connectionStates.testFlag(ConnectionState::DISCONNECTED) ||
            _connectionStates.testFlag(ConnectionState::ERROR_STATE)) {
            QMutexLocker locker(&_sendMutex);
            _sendQueue.clear();
            _currentHeartbeatRetries = 0;
            _waitingForHeartbeatResponse = false;
        }

        LOGGER_DEBUG(std::format("Connection state changed from {} to {}",
            oldStates.toString(),
            _connectionStates.toString()));

        Q_EMIT connectionStateChanged(_connectionStates);
    }
}

void FKTcpManager::_addConnectionState(ConnectionState flag)
{
    // 验证状态组合的合法性
    if ((flag == ConnectionState::AUTHENTICATING || flag == ConnectionState::AUTHENTICATED) &&
        !_connectionStates.testFlag(ConnectionState::CONNECTED)) {
        LOGGER_WARN("Cannot add AUTHENTICATING or AUTHENTICATED state without CONNECTED state");
        return;
    }

    ConnectionStates oldStates = _connectionStates;
    _connectionStates |= flag;

    if (oldStates != _connectionStates) {
        LOGGER_DEBUG(std::format("Added connection state flag: {}", magic_enum::enum_name(flag)));
        Q_EMIT connectionStateChanged(_connectionStates);
    }
}

void FKTcpManager::_removeConnectionState(ConnectionState flag)
{
    ConnectionStates oldStates = _connectionStates;
    _connectionStates &= ~ConnectionStates{ flag };

    if (oldStates != _connectionStates) {
        LOGGER_DEBUG(std::format("Removed connection state flag: {}", magic_enum::enum_name(flag)));
        Q_EMIT connectionStateChanged(_connectionStates);
    }
}

bool FKTcpManager::_hasConnectionState(ConnectionState flag) const
{
    return _connectionStates.testFlag(flag);
}

void FKTcpManager::_startHeartbeat()
{
    // Timer 再哪一个线程定义就必须在哪一个线程启动，这里需要在主线程启动
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "_startHeartbeat", Qt::QueuedConnection);
        return;
    }
    if (_heartbeatInterval > 0) {
        _heartbeatTimeoutCount = 0;
        _currentHeartbeatRetries = 0;
        _waitingForHeartbeatResponse = false;

        // 立即发送第一次心跳
        sendHeartbeat();

        // 启动心跳响应检测定时器
        _heartbeatTimer->start(_heartbeatInterval);
    }
}

void FKTcpManager::_stopHeartbeat()
{
    _heartbeatTimer->stop();  // 停止心跳检测定时器
    _currentHeartbeatRetries = 0;
    _waitingForHeartbeatResponse = false;
    LOGGER_TRACE("Heartbeat stopped");
}

void FKTcpManager::_startReconnectTimer()
{
    if (_reconnectBaseTimeout > 0 && _currentReconnectAttempts < _maxReconnectAttempts) {
        // 指数退避算法
        int baseInterval = _reconnectBaseTimeout / 1000; // 转换为秒
        int maxInterval = 60; // 最大间隔60秒
        int exponent = _currentReconnectAttempts; // 已经尝试的次数
        int interval = baseInterval * (1 << exponent); // 2的exponent次方
        interval = qMin(interval, maxInterval);

        _reconnectTimer->start(interval * 1000);
        LOGGER_DEBUG(std::format("Reconnect timer started, will retry in {} seconds", interval));
    }
}

void FKTcpManager::_stopReconnectTimer()
{
    _reconnectTimer->stop();
}

void FKTcpManager::_emitMessageSignal(MessageType type, const QJsonObject& message)
{
    Q_EMIT messageReceived(type, message);
}

void FKTcpManager::_resetPacketState()
{
    _headerReceived = false;
    _expectedBodyLength = 0;
    std::memset(&_currentHeader, 0, sizeof(_currentHeader));
}
