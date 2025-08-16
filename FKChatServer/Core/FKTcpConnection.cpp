#include "FKTcpConnection.h"
#include "FKChatServer.h"
#include "Flicker/Global/Grpc/FKGrpcServiceClient.hpp"
#include "Library/Logger/logger.h"
#include <nlohmann/json.hpp>
#include <queue>

FKTcpConnection::FKTcpConnection(boost::asio::io_context& ioc, std::shared_ptr<FKChatServer> server)
    : _pIoContext(ioc)
    , _pSocket(ioc)
    , _pTimeout(ioc)
    , _pServer(server)
{
    LOGGER_DEBUG("FKTcpConnection created");
}

FKTcpConnection::~FKTcpConnection()
{
    stop();
    LOGGER_DEBUG("FKTcpConnection destroyed");
}

void FKTcpConnection::start()
{
    if (_pIsClosed.load()) {
        return;
    }

    LOGGER_INFO(std::format("TCP连接开始处理: {}",
        _pSocket.remote_endpoint().address().to_string()));

    // 开始读取消息
    _readMessage();
}

void FKTcpConnection::stop()
{
    if (_pIsClosed.exchange(true)) {
        return;
    }

    LOGGER_INFO("准备关闭TCP连接...");

    // 取消超时计时器
    boost::system::error_code ec;
    _pTimeout.cancel();

    // 关闭socket
    if (_pSocket.is_open()) {
        try {
            _pSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec && ec != boost::asio::error::not_connected) {
                LOGGER_ERROR(std::format("关闭socket错误: {}", ec.message()));
            }

            _pSocket.close(ec);
            if (ec) {
                LOGGER_ERROR(std::format("关闭socket错误: {}", ec.message()));
            }
        }
        catch (const boost::system::system_error& ex) {
            LOGGER_ERROR(std::format("关闭socket异常: {}", ex.what()));
        }
    }

    // 调用关闭回调
    _closeConnection();
}

void FKTcpConnection::sendMessage(const std::string& message, Flicker::Tcp::MessageType type)
{
    if (_pIsClosed.load()) {
        LOGGER_WARN("连接已关闭，无法发送消息");
        return;
    }

    // 检查写入队列大小限制
    {
        std::lock_guard<std::mutex> lock(_pSendMutex);
        if (_pSendQueue.size() >= MAX_WRITE_QUEUE) {
            LOGGER_WARN(std::format("写入队列已满，丢弃消息，用户: {}", _pUserUuid));
            return;
        }
    }
    _sendMessage(message, type);
}

void FKTcpConnection::_readMessage()
{
    if (_pIsClosed.load()) {
        return;
    }

    auto self = shared_from_this();
    const size_t currentSize = _pReceiveBuffer.size();

    if (_pReceiveBuffer.capacity() - currentSize < MIN_BUFFER_SIZE) {
        // 计算实际需要空间
        _pReceiveBuffer.reserve(currentSize + std::max(MIN_BUFFER_SIZE, _pExpectedBodyLength));
    }

    // 调整大小以容纳新数据
    const size_t readSize = _pReceiveBuffer.capacity() - currentSize;
    _pReceiveBuffer.resize(currentSize + readSize);

    // 异步读取数据
    _pSocket.async_read_some(
        boost::asio::buffer(_pReceiveBuffer.data() + currentSize,
            readSize),
        [self, currentSize](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                if (ec == boost::asio::error::eof) {
                    LOGGER_INFO("客户端关闭连接");
                }
                else if (ec == boost::asio::error::operation_aborted) {
                    LOGGER_INFO("读取操作被取消");
                }
                else {
                    LOGGER_ERROR(std::format("TCP读取错误: {}", ec.message()));
                }

                self->stop();
                return;
            }

            // 更新接收缓冲区大小
            self->_pReceiveBuffer.resize(currentSize + bytes_transferred);

            LOGGER_TRACE(std::format("收到数据: {} 字节", bytes_transferred));

            // 处理接收到的数据
            self->_processReceivedData();

            // 继续读取
            self->_readMessage();
        }
    );
}

void FKTcpConnection::_processReceivedData()
{
    const size_t bufferSize = _pReceiveBuffer.size();
    // 保存原始处理位置
    const size_t initialProcessed = _pReceiveProcessed;

    while (_pReceiveProcessed < bufferSize && !_pIsClosed) {
        // 如果还没有接收到完整的消息头
        if (!_pHeaderReceived) {
            // 检查是否有足够数据解析消息头
            if (bufferSize - _pReceiveProcessed < sizeof(Flicker::Tcp::MessageHeader)) {
                break; // 数据不够，等待更多数据
            }

            // 直接从缓冲区拷贝消息头
            std::memcpy(&_pCurrentHeader,
                _pReceiveBuffer.data() + _pReceiveProcessed,
                sizeof(Flicker::Tcp::MessageHeader));

            // 移动到消息头之后的位置
            _pReceiveProcessed += sizeof(Flicker::Tcp::MessageHeader);

            if (!_parseMessageHeader()) {
                LOGGER_ERROR("解析消息头失败");
                _sendErrorMessage("Invalid message header");
                stop();
                return;
            }
        }


        // 检查是否接收到完整的消息体
        if (_pReceiveBuffer.size() < (sizeof(Flicker::Tcp::MessageHeader) + _pExpectedBodyLength)) {
            break; // 数据不够，等待更多数据
        }

         // 检查是否接收到完整的消息体
        const size_t remaining = bufferSize - _pReceiveProcessed;
        if (remaining < _pExpectedBodyLength) {
            break; // 数据不够，等待更多数据
        }

        // 提取消息体 - 直接使用vector中的数据
        const char* bodyStart = reinterpret_cast<const char*>(
            _pReceiveBuffer.data() + _pReceiveProcessed);
        std::string messageBody(bodyStart, _pExpectedBodyLength);
        
        // 移动到消息体之后的位置
        _pReceiveProcessed += _pExpectedBodyLength;

        // 处理完整消息
        _handleCompleteMessage(_pCurrentHeader, messageBody);

        // 重置状态，准备处理下一个消息
        _resetPacketState();
    }

    // 移除已处理的数据
    if (_pReceiveProcessed > initialProcessed) {
        const size_t processedCount = _pReceiveProcessed - initialProcessed;
        const size_t unprocessed = bufferSize - _pReceiveProcessed;
        if (_pReceiveProcessed + unprocessed > bufferSize) {
            LOGGER_ERROR("缓冲区处理溢出!");
            stop();
            return;
        }
        if (unprocessed > 0) {
            // 将剩余未处理数据移动到缓冲区开头
            std::memmove(_pReceiveBuffer.data(),
                _pReceiveBuffer.data() + _pReceiveProcessed,
                unprocessed);
        }

        // 调整缓冲区大小
        _pReceiveBuffer.resize(unprocessed);
        _pReceiveProcessed = 0; // 重置为缓冲区起始位置
        
        LOGGER_TRACE(std::format("压缩接收缓冲区: 已处理{}字节, 剩余{}字节",
            processedCount, unprocessed));
    }
}

bool FKTcpConnection::_parseMessageHeader()
{
    // 验证魔数
    if (_pCurrentHeader.magic != MESSAGE_MAGIC) {
        LOGGER_ERROR(std::format("无效的魔数: 0x{:x}", _pCurrentHeader.magic));
        return false;
    }

    // 验证消息长度
    if (_pCurrentHeader.length > MAX_MESSAGE_SIZE) {
        LOGGER_ERROR(std::format("消息过大: {}", _pCurrentHeader.length));
        return false;
    }

    // 验证协议版本
    if (_pCurrentHeader.version != PROTOCOL_VERSION) {
        LOGGER_ERROR(std::format("不支持的协议版本: {}", _pCurrentHeader.version));
        return false;
    }

    _pExpectedBodyLength = _pCurrentHeader.length;
    _pHeaderReceived = true;
    return true;
}

void FKTcpConnection::_resetPacketState()
{
    _pHeaderReceived = false;
    _pExpectedBodyLength = 0;
    memset(&_pCurrentHeader, 0, sizeof(_pCurrentHeader));
}

void FKTcpConnection::_handleCompleteMessage(const Flicker::Tcp::MessageHeader& header, const std::string& body)
{
    Flicker::Tcp::MessageType messageType = static_cast<Flicker::Tcp::MessageType>(header.type);

    LOGGER_DEBUG(std::format("处理消息类型: {}", static_cast<int>(messageType)));

    _processMessage(messageType, body);
}

void FKTcpConnection::_processMessage(Flicker::Tcp::MessageType messageType, const std::string& messageBody)
{
    switch (messageType) {
    case Flicker::Tcp::MessageType::AUTH_REQUEST:
        _handleAuthRequest(messageBody);
        break;
    case Flicker::Tcp::MessageType::HEARTBEAT:
        _handleHeartbeat(messageBody);
        break;
    case Flicker::Tcp::MessageType::CHAT_MESSAGE:
        _handleChatMessage(messageBody);
        break;
    default:
        LOGGER_WARN(std::format("未知消息类型: {}", static_cast<int>(messageType)));
        _sendErrorMessage("Unknown message type");
        break;
    }
}

void FKTcpConnection::_handleAuthRequest(const std::string& messageBody)
{
    try {
        // 解析JSON消息
        auto json = nlohmann::json::parse(messageBody);

        if (!json.contains("token") || !json.contains("client_device_id")) {
            LOGGER_ERROR("认证请求缺少必要字段");
            _sendAuthResponse(false, "Missing required fields");
            return;
        }

        std::string token = json["token"];
        std::string clientDeviceId = json["client_device_id"];

        LOGGER_INFO(std::format("收到认证请求，设备ID: {}", clientDeviceId));

        // 验证token
        if (_validateToken(token, clientDeviceId)) {
            _pClientDeviceId = clientDeviceId;
            _pIsAuthenticated.store(true);

            // 添加到服务器连接管理
            if (auto server = _pServer.lock()) {
                server->addConnection(_pUserUuid, shared_from_this());
            }

            LOGGER_INFO(std::format("用户认证成功: {}", _pUserUuid));
            // 认证成功后重置定时器，启动心跳检测
            _checkTimeout();
            _sendAuthResponse(true, "Authentication successful");
        }
        else {
            LOGGER_WARN("Token验证失败");
            _sendAuthResponse(false, "Invalid token");
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("处理认证请求异常: {}", e.what()));
        _sendAuthResponse(false, "Authentication error");
    }
}

void FKTcpConnection::_handleHeartbeat(const std::string& messageBody)
{
    if (!_pIsAuthenticated.load()) {
        LOGGER_WARN("未认证用户发送心跳");
        _sendErrorMessage("Not authenticated");
        return;
    }

    LOGGER_TRACE("收到心跳消息");
    // 重置心跳超时定时器
    _checkTimeout();
    _sendHeartbeatResponse();
}

void FKTcpConnection::_handleChatMessage(const std::string& messageBody)
{
    if (!_pIsAuthenticated.load()) {
        LOGGER_WARN("未认证用户发送聊天消息");
        _sendErrorMessage("Not authenticated");
        return;
    }

    try {
        auto json = nlohmann::json::parse(messageBody);
        std::string content = json["content"];

        LOGGER_INFO(std::format("收到聊天消息: {}", content));

        // 这里可以添加消息转发逻辑
        // 例如：转发给其他用户或群组

    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("处理聊天消息异常: {}", e.what()));
        _sendErrorMessage("Message processing error");
    }
}

bool FKTcpConnection::_validateToken(const std::string& token, const std::string& clientDeviceId)
{
    try {
        // 使用gRPC客户端验证token
        FKGrpcServiceClient<Flicker::Server::Enums::GrpcServiceType::ValidateToken> client;

        im::service::ValidateTokenRequest request;
        request.set_token(token);
        request.set_client_device_id(clientDeviceId);

        auto [response, status] = client.validateToken(request);

        if (!status.ok()) {
            LOGGER_ERROR(std::format("gRPC调用失败: {}", status.error_message()));
            return false;
        }

        if (response.status() == im::service::StatusCode::ok) {
            _pUserUuid = response.user_uuid();
            LOGGER_INFO(std::format("Token验证成功，用户UUID: {}", _pUserUuid));
            return true;
        }
        else {
            LOGGER_WARN(std::format("Token验证失败: {}", response.error_detail()));
            return false;
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("Token验证异常: {}", e.what()));
        return false;
    }
}

void FKTcpConnection::_sendMessage(const std::string& data, Flicker::Tcp::MessageType type)
{
    // 构建消息头
    Flicker::Tcp::MessageHeader header;
    header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    header.magic = MESSAGE_MAGIC;
    header.length = static_cast<uint32_t>(data.size());
    header.type = static_cast<uint16_t>(type);
    header.version = PROTOCOL_VERSION;
    header.reserved = 0;

    // 组装完整消息
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(header) + data.size());

    // 添加头部
    const uint8_t* header_ptr = reinterpret_cast<const uint8_t*>(&header);
    buffer.insert(buffer.end(), header_ptr, header_ptr + sizeof(header));

    // 添加数据
    const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(data.data());
    buffer.insert(buffer.end(), data_ptr, data_ptr + data.size());

    {
        std::lock_guard<std::mutex> lock(_pSendMutex);
        _pSendQueue.push(std::move(buffer));
    }

    if (!_pIsSending) {
        _pIsSending = true;
        _writeMessage();
    }
}

void FKTcpConnection::_writeMessage()
{
    if (_pIsClosed.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(_pSendMutex);

    if (_pSendQueue.empty()) {
        _pIsSending = false;
        return;
    }

    auto self = shared_from_this();
    const std::vector<uint8_t>& message = _pSendQueue.front();

    boost::asio::async_write(
        _pSocket,
        boost::asio::buffer(message),
        [self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                LOGGER_ERROR(std::format("发送消息错误: {}", ec.message()));
                self->stop();
                return;
            }

            LOGGER_TRACE(std::format("发送消息成功: {} 字节", bytes_transferred));

            {
                std::lock_guard<std::mutex> lock(self->_pSendMutex);
                self->_pSendQueue.pop();

                if (!self->_pSendQueue.empty()) {
                    // 继续发送下一个消息
                    self->_writeMessage();
                }
                else {
                    self->_pIsSending = false;
                }
            }
        }
    );
}

void FKTcpConnection::_sendAuthResponse(bool success, const std::string& message)
{
    nlohmann::json response;
    response["success"] = success;
    response["message"] = message;
    if (success && !_pUserUuid.empty()) {
        response["user_uuid"] = _pUserUuid;
    }
    _sendMessage(response.dump(), Flicker::Tcp::MessageType::AUTH_RESPONSE);
}

void FKTcpConnection::_sendHeartbeatResponse()
{
    nlohmann::json response;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    response["status"] = "ok";

    _sendMessage(response.dump(), Flicker::Tcp::MessageType::HEARTBEAT);
}

void FKTcpConnection::_sendErrorMessage(const std::string& error)
{
    nlohmann::json response;
    response["error"] = error;

    _sendMessage(response.dump(), Flicker::Tcp::MessageType::ERROR_MESSAGE);
}

void FKTcpConnection::_checkTimeout()
{
    // 根据认证状态设置不同的超时时间
    auto timeout = _pIsAuthenticated.load() ? HEARTBEAT_TIMEOUT : AUTH_TIMEOUT;
    _pTimeout.expires_after(timeout);

    auto self = shared_from_this();
    _pTimeout.async_wait(
        [self](boost::system::error_code ec) {
            if (!ec) {
                if (self->_pIsAuthenticated.load()) {
                    LOGGER_WARN("心跳超时，关闭连接");
                } else {
                    LOGGER_WARN("认证超时，关闭连接");
                }
                self->stop();
            }
        }
    );
}

void FKTcpConnection::_closeConnection()
{
    if (_pCloseCallback) {
        _pCloseCallback(_pUserUuid);
    }

    // 从服务器连接管理中移除
    if (auto server = _pServer.lock()) {
        server->removeConnection(_pUserUuid);
    }
}