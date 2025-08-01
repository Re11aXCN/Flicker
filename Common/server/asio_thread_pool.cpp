﻿#include "Common/server/asio_ioc_thread_pool.h"
#include "Common/config/config_manager.h"
#include "Common/logger/logger_defend.h"

SINGLETON_CREATE_CPP(AsioIocThreadPool)
AsioIocThreadPool::AsioIocThreadPool()
    : _pIsRunning(false)
    , _pNextIndex(0)
{
    // 使用配置的参数初始化线程池
    _initialize(ConfigManager::getInstance()->getAsioIocThreadPoolConfig());
}

AsioIocThreadPool::~AsioIocThreadPool()
{
    stop();
}

void AsioIocThreadPool::_initialize(const AsioIocThreadPoolConfig& config)
{
    if (_pIsRunning) return;
    LOGGER_INFO("线程池初始化中...");

    _pIsRunning = true;
    std::vector<ioContext> newContexts{ config.ThreadCount };
    _pContexts.swap(newContexts);
    _pGuards.clear();
    _pGuards.reserve(config.ThreadCount);
    _pThreads.resize(config.ThreadCount);

    // 创建任务通道
    _pTaskChannels.resize(3); // 高、中、低三个优先级
    for (auto& channel : _pTaskChannels) {
        channel = std::make_unique<taskChannel>(_pContexts[0], config.ChannelCapacity);
    }

    // 为每个io_context创建工作守卫和线程
    for (size_t i = 0; i < config.ThreadCount; ++i) {
        // 创建工作守卫
        _pGuards.emplace_back(boost::asio::make_work_guard(_pContexts[i]));

        // 创建线程运行io_context
        _pThreads[i] = std::make_unique<std::thread>([this, i]() {
            try {
                // 设置线程名称
                this->_setThreadName(std::format("ioThread-{}", i));

                // 运行io_context
                _pContexts[i].run();
            }
            catch (const std::exception& e) {
                LOGGER_ERROR(std::format("线程池运行异常: {}", e.what()));
            }
            });
    }

    // 启动任务分发器
    _startTaskDispatcher();
    LOGGER_INFO("线程池初始化完成!");
}

void AsioIocThreadPool::stop()
{
    if (!_pIsRunning) return;

    // 关闭任务通道
    for (auto& channel : _pTaskChannels) {
        if (channel) {
            channel->close();
        }
    }

    // 移除工作守卫，允许io_context退出
    for (auto& guard : _pGuards) {
        guard.reset();
    }

    // 停止所有io_context
    for (auto& context : _pContexts) {
        context.stop();
    }

    // 等待所有线程结束
    for (auto& thread : _pThreads) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }

    // 清理资源
    _pThreads.clear();
    _pGuards.clear();
    _pContexts.clear();
    _pTaskChannels.clear();
    _pIsRunning = false;
}

AsioIocThreadPool::ioContext& AsioIocThreadPool::getNextContext()
{
    if (_pContexts.empty()) {
        throw std::runtime_error("vector<ioContext> is empty! maybe you should call initialize() first!" __FUNCTION__ "");
    }
    // 使用原子操作实现线程安全的轮询
    size_t current = _pNextIndex.fetch_add(1, std::memory_order_relaxed);
    return _pContexts[current % _pContexts.size()];
}

AsioIocThreadPool::ioContext& AsioIocThreadPool::getContext(size_t index)
{
    return _pContexts[index % _pContexts.size()];
}

bool AsioIocThreadPool::post(std::function<void()> task, Priority priority /*= Priority::normal*/)
{
    if (!_pIsRunning || !task) return false;

    // 轮询选择通道
    size_t priorityIndex = _pNextIndex.fetch_add(1, std::memory_order_relaxed) % _pTaskChannels.size();
    if (!_pTaskChannels[priorityIndex]) return false;

    // 异步发送任务到对应优先级的通道
    _pTaskChannels[priorityIndex]->async_send(
        boost::system::error_code{},
        std::move(task),
        [](boost::system::error_code ec) {
            if (ec) {
                LOGGER_ERROR(std::format("任务提交失败: {}", ec.message()));
            }
        }
    );

    return true;
}

void AsioIocThreadPool::_setThreadName(const std::string& name)
{
#if defined(__linux__)
    pthread_setname_np(pthread_self(), name.c_str());
#elif defined(_WIN32)
    // Windows线程命名 - 使用结构化异常处理
    const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
    struct THREADNAME_INFO {
        DWORD dwType;     // 必须为0x1000
        LPCSTR szName;    // 线程名称指针
        DWORD dwThreadID; // 线程ID (-1表示调用线程)
        DWORD dwFlags;    // 保留，必须为0
    };
#pragma pack(pop)
    THREADNAME_INFO info{
        0x1000,
        name.c_str(),
        GetThreadId(GetCurrentThread()),
        0
    };
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
}

void AsioIocThreadPool::_startTaskDispatcher()
{
    // 为每个优先级通道创建接收器
    for (size_t i = 0; i < _pTaskChannels.size(); ++i) {
        if (!_pTaskChannels[i]) continue;

         // 使用 std::function 声明接收函数
        std::function<void()> receiveFunc;

        // 定义接收函数实现
        receiveFunc = [this, i, &receiveFunc]() {
            _pTaskChannels[i]->async_receive(
                [this, i, &receiveFunc](boost::system::error_code ec, std::function<void()> task) {
                    if (ec) {
                        if (ec != boost::asio::experimental::channel_errc::channel_closed) {
                            LOGGER_ERROR(std::format("任务接收错误: {}", ec.message()));
                        }
                        return;
                    }

                    // 直接使用当前通道绑定的io_context
                    boost::asio::post(_pContexts[i], [task = std::move(task)]() {
                        try {
                            if (task) task();
                        }
                        catch (const std::exception& e) {
                            LOGGER_ERROR(std::format("任务执行异常: {}", e.what()));
                        }
                        });

                    // 继续接收下一个任务
                    receiveFunc();
                }
            );
            };

        // 开始接收任务
        receiveFunc();
    }
}
