#include "FKServer.h"

#include "Asio/FKHttpConnection.h"
#include "Asio/FKAsioThreadPool.h"
#include "FKLogger-Defend.h"

FKServer::FKServer(boost::asio::io_context& ioc, UINT16 port)
    : _pIoContext(ioc)
    , _pAcceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{

}

void FKServer::stop()
{
    FK_SERVER_INFO("服务器正在停止...");
    // 标记服务器为非运行状态
    _pIsRunning = false;
    
    // 关闭接收器，停止接受新连接
    boost::system::error_code ec;
    _pAcceptor.close(ec);
    if (ec) {
        FK_SERVER_ERROR(std::format("关闭接收器错误: {}", ec.message()));
    }
    
    // 等待所有活动连接完成
    FK_SERVER_INFO(std::format("正在等待 {} 个活动连接完成...", _pActiveConnections.load()));
    
    // 使用轮询方式等待活动连接完成
    // 在实际应用中，可以使用条件变量或其他同步机制
    while (_pActiveConnections > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    FK_SERVER_INFO("服务器已停止!");
}

void FKServer::_incrementActiveConnections()
{
    ++_pActiveConnections;
}

void FKServer::_decrementActiveConnections()
{
    --_pActiveConnections;
    FK_SERVER_TRACE("活动连接数: {}", _pActiveConnections.load());
}

void FKServer::start()
{
    // 标记服务器为运行状态
    _pIsRunning = true;
    try {
        // 获取下一个可用的IO上下文
        FKAsioThreadPool::ioContext& ioc = FKAsioThreadPool::getInstance()->getNextContext();
        
        // 创建新的HTTP连接对象
        std::shared_ptr<FKHttpConnection> connection = std::make_shared<FKHttpConnection>(ioc);
        
        // 异步接受新连接
        _pAcceptor.async_accept(
            connection->getSocket(), 
            [self = shared_from_this(), connection](boost::beast::error_code ec) {
                try {
                    // 处理接受连接时的错误
                    if (ec) {
                        FK_SERVER_ERROR(std::format("接受连接错误: {}", ec.message()));

                        // 短暂延迟后继续监听，避免在错误情况下的快速循环
                        boost::asio::steady_timer timer(self->_pIoContext);
                        timer.expires_after(std::chrono::milliseconds(100));
                        timer.async_wait([self](const boost::system::error_code&) {
                            if (self->_pIsRunning) {
                                self->start();
                            }
                        });
                        return;
                    }

                    // 如果服务器已停止，不处理新连接
                    if (!self->_pIsRunning) {
                        FK_SERVER_TRACE("服务器已停止，拒绝新连接");

                        boost::system::error_code close_ec;
                        connection->getSocket().close(close_ec);
                        return;
                    }
                    
                    // 处理新连接
                    FK_SERVER_INFO(std::format("接受新连接: {}",
                        connection->getSocket().remote_endpoint().address().to_string()));
                    
                    // 增加活动连接计数
                    self->_incrementActiveConnections();
                    
                    // 设置连接关闭回调
                    connection->setCloseCallback([self]() {
                        self->_decrementActiveConnections();
                    });
                    
                    connection->start();
                    
                    // 如果服务器仍在运行，继续监听新连接
                    if (self->_pIsRunning) {
                        self->start();
                    }
                }
                catch (const std::exception& ex) {
                    FK_SERVER_ERROR(std::format("处理连接异常: {}", ex.what()));
                    
                    // 继续监听新连接
                    if (self->_pIsRunning) {
                        self->start();
                    }
                }
                catch (...) {
                    FK_SERVER_ERROR("处理连接时发生未知异常");
                    if (self->_pIsRunning) {
                        self->start();
                    }
                }
            });
    }
    catch (const std::exception& ex) {
        FK_SERVER_ERROR(std::format("服务器启动异常: {}", ex.what()));

        // 短暂延迟后重试
        boost::asio::steady_timer timer(_pIoContext);
        timer.expires_after(std::chrono::milliseconds(1));
        timer.async_wait([self = shared_from_this()](const boost::system::error_code&) {
            if (self->_pIsRunning) {
                self->start();
            }
        });
    }
}
