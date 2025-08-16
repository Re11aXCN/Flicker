#include "FKHttpConnection.h"

#include "FKLogicSystem.h"
#include "Flicker/Global/universal/utils.h"
#include "Library/Logger/logger.h"

FKHttpConnection::FKHttpConnection(boost::asio::io_context& ioc)
    : _pSocket(ioc)
    , _pBuffer{ 8192 }
    , _pTimeout{ _pSocket.get_executor(), std::chrono::milliseconds(60) }
{

}

FKHttpConnection::~FKHttpConnection()
{
    // 确保连接被关闭
    stop();
}

void FKHttpConnection::stop()
{
    // 如果连接已经关闭，直接返回
    if (_pIsClosed.exchange(true)) {
        return;
    }

    LOGGER_INFO("准备关闭HTTP连接...");
    
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
        } catch (const boost::system::system_error& ex) {
            LOGGER_ERROR(std::format("关闭socket异常: {}", ex.what()));
        }
    }
    
    // 调用关闭回调函数
    _closeConnection();
}

void FKHttpConnection::_closeConnection()
{
    // 调用关闭回调函数
    if (_pCloseCallback) {
        _pCloseCallback();
    }
}

void FKHttpConnection::start()
{
    // 设置连接超时检查
    _checkTimeout();

    // 异步读取HTTP请求
    boost::beast::http::async_read(
        _pSocket, 
        _pBuffer, 
        _pRequest, 
        [self = shared_from_this()](boost::beast::error_code ec, size_t bytes_transferred) {
            try {
                // 处理读取错误
                if (ec) {
                    if (ec == boost::beast::http::error::end_of_stream) {
                        LOGGER_INFO("客户端关闭连接");
                    } else if (ec == boost::asio::error::operation_aborted) {
                        LOGGER_INFO("读取操作被取消");
                    } else {
                        LOGGER_ERROR(std::format("HTTP读取错误: {}", ec.message()));
                    }
                    
                    // 关闭连接
                    self->stop();
                    return;
                }

                // 处理读取到的数据
                boost::ignore_unused(bytes_transferred);
                LOGGER_INFO(std::format("收到HTTP请求: {} {}", 
                    self->_pRequest.method_string(), 
                    self->_pRequest.target()));
                
                // 处理请求并重置超时计时器
                self->_handleRequest();
            }
            catch (const std::exception& ex) {
                LOGGER_ERROR(std::format("处理HTTP请求异常: {}", ex.what()));
                
                // 发送错误响应
                self->_pResponse.result(boost::beast::http::status::internal_server_error);
                self->_pResponse.set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(self->_pResponse.body()) << "服务器内部错误\r\n";
                self->_writeResponse();
            }
            catch (...) {
                LOGGER_ERROR("处理HTTP请求时发生未知异常");
                
                // 发送错误响应
                self->_pResponse.result(boost::beast::http::status::internal_server_error);
                self->_pResponse.set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(self->_pResponse.body()) << "服务器内部错误\r\n";
                self->_writeResponse();
            }
        }
    );
}

void FKHttpConnection::_checkTimeout()
{
    // 重置超时计时器
    _pTimeout.expires_after(std::chrono::milliseconds(1000));
    
    // 异步等待超时
    _pTimeout.async_wait(
        [self = shared_from_this()](boost::beast::error_code ec)
        {
            // 如果计时器未被取消且正常触发
            if (!ec)
            {
                LOGGER_TRACE("连接超时，关闭socket");
                
                // 关闭连接
                self->stop();
            }
            else if (ec != boost::asio::error::operation_aborted) {
                // 如果不是因为取消操作导致的错误
                LOGGER_ERROR(std::format("超时检查错误: {}", ec.message()));
            }
        });
}

void FKHttpConnection::_writeResponse()
{
    // 设置内容长度和常见响应头
    _pResponse.content_length(_pResponse.body().size());
    _pResponse.set(boost::beast::http::field::server, "GateServer");
    _pResponse.set(boost::beast::http::field::date, universal::utils::time::get_gmtime());
    
    // 设置为短连接
    _pResponse.keep_alive(false);
    
    // 异步写入响应
    boost::beast::http::async_write(
        _pSocket,
        _pResponse,
        [self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            // 取消超时计时器
            self->_pTimeout.cancel();
            
            // 处理写入错误
            if (ec) {
                LOGGER_ERROR(std::format("写入响应错误: {}", ec.message()));
                return;
            }
            
            LOGGER_INFO(std::format("响应已发送: {} 字节, 状态: {}", 
                bytes_transferred, 
                self->_pResponse.result_int()));
            
            // 尝试优雅关闭连接
            try {
                // 关闭发送端
                boost::beast::error_code shutdown_ec;
                self->_pSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, shutdown_ec);
                if (shutdown_ec && shutdown_ec != boost::asio::error::not_connected) {
                    LOGGER_ERROR(std::format("关闭发送端错误: {}", shutdown_ec.message()));
                }
            } catch (const std::exception& ex) {
                LOGGER_ERROR(std::format("关闭连接异常: {}", ex.what()));
            }
        });
}

void FKHttpConnection::_handleRequest()
{
    try {
        // 设置响应版本和连接类型
        _pResponse.version(_pRequest.version());
        _pRequest.keep_alive(false);
        
        // 记录请求信息
        LOGGER_INFO(std::format("处理HTTP请求: {} {}", 
            _pRequest.method_string(), 
            _pRequest.target()));
        
        // 根据HTTP方法处理请求
        bool success = false;
        switch (_pRequest.method())
        {
        case boost::beast::http::verb::get: {
            // 解析GET请求的查询参数
            _pQueryParams = universal::utils::miscella::parse_query_params(_pRequest.target(), _pUrl);
            
            // 记录解析后的URL和参数数量
            LOGGER_INFO(std::format("解析URL: {}, 参数数量: {}", _pUrl, _pQueryParams.size()));
            
            // 调用业务逻辑处理
            success = FKLogicSystem::getInstance()->callBack(_pUrl, boost::beast::http::verb::get, shared_from_this());
            break;
        }
        case boost::beast::http::verb::post: {
            // POST请求直接传递完整target给业务逻辑
            _pUrl = std::string(_pRequest.target());
            
            // 记录请求体大小
            LOGGER_INFO(std::format("POST请求: {}, 请求体大小: {}", _pUrl, _pRequest.body().size()));
            
            // 调用业务逻辑处理
            success = FKLogicSystem::getInstance()->callBack(_pUrl, boost::beast::http::verb::post, shared_from_this());
            break;
        }
        case boost::beast::http::verb::options: {
            // 处理OPTIONS请求（CORS预检请求）
            LOGGER_INFO(std::format("处理OPTIONS请求: {}", _pRequest.target()));
            
            // 设置CORS响应头
            _pResponse.result(boost::beast::http::status::no_content);
            _pResponse.set(boost::beast::http::field::access_control_allow_origin, "*");
            _pResponse.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            _pResponse.set(boost::beast::http::field::access_control_allow_headers, "Content-Type");
            _pResponse.set(boost::beast::http::field::access_control_max_age, "86400");
            
            // 直接发送响应
            _writeResponse();
            return;
        }
        default:
            // 不支持的HTTP方法
            LOGGER_ERROR(std::format("不支持的HTTP方法: {}", _pRequest.method_string()));
            
            // 设置405响应
            _pResponse.result(boost::beast::http::status::method_not_allowed);
            _pResponse.set(boost::beast::http::field::content_type, "application/json");
            _pResponse.set(boost::beast::http::field::allow, "GET, POST, OPTIONS");
            boost::beast::ostream(_pResponse.body()) << 
                "{\"code\":405,\"message\":\"Method Not Allowed\"}";
            _writeResponse();
            return;
        }

        // 处理URL未找到的情况
        if (!success) {
            LOGGER_TRACE(std::format("URL未找到: {}", _pUrl));
            
            // 设置404响应
            _pResponse.result(boost::beast::http::status::not_found);
            _pResponse.set(boost::beast::http::field::content_type, "application/json");
            boost::beast::ostream(_pResponse.body()) << 
                "{\"code\":404,\"message\":\"Not Found\",\"path\":\"" << 
                _pUrl << "\"}";
            _writeResponse();
            return;
        }
        
        // 如果业务逻辑已经设置了响应状态码，则不再修改
        if (_pResponse.result() == boost::beast::http::status::ok) {
            // 默认设置为200 OK
            _pResponse.result(boost::beast::http::status::ok);
        }
        
        // 发送响应
        _writeResponse();
    }
    catch (const std::exception& ex) {
        // 处理请求处理过程中的异常
        LOGGER_ERROR(std::format("处理请求异常: {}", ex.what()));
        
        // 设置500错误响应
        _pResponse.result(boost::beast::http::status::internal_server_error);
        _pResponse.set(boost::beast::http::field::content_type, "application/json");
        boost::beast::ostream(_pResponse.body()) << 
            "{\"code\":500,\"message\":\"Internal Server Error\",\"error\":\"" << 
            ex.what() << "\"}";
        _writeResponse();
    }
    catch (...) {
        // 处理未知异常
        LOGGER_ERROR("处理请求时发生未知异常");
        
        // 设置500错误响应
        _pResponse.result(boost::beast::http::status::internal_server_error);
        _pResponse.set(boost::beast::http::field::content_type, "application/json");
        boost::beast::ostream(_pResponse.body()) << 
            "{\"code\":500,\"message\":\"Internal Server Error\",\"error\":\"Unknown error\"}";
        _writeResponse();
    }
}