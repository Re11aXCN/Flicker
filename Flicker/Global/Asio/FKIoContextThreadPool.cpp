#include "FKIoContextThreadPool.h"
#include "Library/Logger/logger.h"

#define DEFAULT_CHANNEL_CAPACITY 1024

SINGLETON_CREATE_CPP(FKIoContextThreadPool)
FKIoContextThreadPool::FKIoContextThreadPool()
    : _pIsRunning(false)
    , _pNextIndex(0)
{
    // 使用配置的参数初始化线程池
    _initialize();
}

FKIoContextThreadPool::~FKIoContextThreadPool()
{
    stop();
}

void FKIoContextThreadPool::_initialize()
{
    if (_pIsRunning.load()) return;
    LOGGER_INFO("线程池初始化中...");

    _pIsRunning.store(true);
    auto threadCount = std::thread::hardware_concurrency() / 2;
    std::vector<ioContext> newContexts{ threadCount };
    _pContexts.swap(newContexts);
    _pGuards.clear();
    _pGuards.reserve(threadCount);
    _pThreads.resize(threadCount);

    // 创建任务通道
    _pTaskChannels.resize(3); // 高、中、低三个优先级
    for (size_t i = 0; i < 3; ++i) {
        _pTaskChannels[i] = std::make_unique<taskChannel>(_pContexts[0], DEFAULT_CHANNEL_CAPACITY);
    }

    // 为每个io_context创建工作守卫和线程
    for (size_t i = 0; i < threadCount; ++i) {
        // 创建工作守卫
        _pGuards.emplace_back(boost::asio::make_work_guard(_pContexts[i]));

        // 创建线程运行io_context
        _pThreads[i] = std::make_unique<std::thread>([this, i]() {
            try {
                // 设置线程名称
                this->_setThreadName(std::format("ioThread-{}", i));

                // 运行io_context
                _pContexts[i].run();

                LOGGER_DEBUG(std::format("线程 {} 正常退出", i));
            }
            catch (const std::exception& e) {
                LOGGER_ERROR(std::format("线程池运行异常: {}", e.what()));
            }
            });
    }

    // 启动任务分发器
    _startTaskDispatcher();
    LOGGER_INFO(std::format("线程池初始化完成! 线程数: {}", threadCount));
}

void FKIoContextThreadPool::stop()
{
    if (!_pIsRunning.exchange(false)) return;
    LOGGER_INFO("线程池开始停止...");

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
    _pPendingTasks.store(0);
}

FKIoContextThreadPool::ioContext& FKIoContextThreadPool::getNextContext()
{
    if (_pContexts.empty()) {
        throw std::runtime_error("vector<ioContext> is empty! maybe you should call initialize() first!" __FUNCTION__ "");
    }
    // 使用原子操作实现线程安全的轮询
    size_t current = _pNextIndex.fetch_add(1, std::memory_order_relaxed);
    return _pContexts[current % _pContexts.size()];
}

FKIoContextThreadPool::ioContext& FKIoContextThreadPool::getContext(size_t index)
{
    return _pContexts[index % _pContexts.size()];
}

bool FKIoContextThreadPool::post(std::function<void()> task, Priority priority /*= Priority::normal*/)
{
    if (!_pIsRunning.load() || !task) return false;

    // 轮询选择通道
    size_t priorityIndex = _pNextIndex.fetch_add(1, std::memory_order_relaxed) % _pTaskChannels.size();
    if (!_pTaskChannels[priorityIndex]) return false;

    // 异步发送任务到对应优先级的通道
    _pTaskChannels[priorityIndex]->async_send(
        boost::system::error_code{},
        std::move(task),
        [this](boost::system::error_code ec) {
            if (ec && ec != boost::asio::experimental::channel_errc::channel_closed) {
                LOGGER_ERROR(std::format("任务提交失败: {}", ec.message()));
                _pPendingTasks.fetch_sub(1, std::memory_order_relaxed);
            }
        }
    );

    return true;
}

int FKIoContextThreadPool::getCurrentLoad() const
{
    if (_pContexts.empty()) return 0;

    size_t pendingTasks = _pPendingTasks.load();
    size_t maxCapacity = _pContexts.size() * DEFAULT_CHANNEL_CAPACITY;

    return static_cast<int>((pendingTasks * 100) / maxCapacity);
}

bool FKIoContextThreadPool::waitForCompletion(uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(_pStatsMutex);

    if (timeoutMs == 0) {
        _pCompletionCV.wait(lock, [this] { return _pPendingTasks.load() == 0; });
        return true;
    }
    else {
        return _pCompletionCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
            [this] { return _pPendingTasks.load() == 0; });
    }
}

void FKIoContextThreadPool::_setThreadName(const std::string& name)
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

//void FKIoContextThreadPool::_startTaskDispatcher()
//{
//    // 为每个优先级通道创建接收器
//    for (size_t i = 0; i < _pTaskChannels.size(); ++i) {
//        if (!_pTaskChannels[i]) continue;
//
//         // 使用 std::function 声明接收函数
//        std::function<void()> receiveFunc;
//
//        // 定义接收函数实现
//        receiveFunc = [this, i, &receiveFunc]() {
//            _pTaskChannels[i]->async_receive(
//                [this, i, &receiveFunc](boost::system::error_code ec, std::function<void()> task) {
//                    if (ec) {
//                        if (ec != boost::asio::experimental::channel_errc::channel_closed) {
//                            LOGGER_ERROR(std::format("任务接收错误: {}", ec.message()));
//                        }
//                        return;
//                    }
//
//                    // 直接使用当前通道绑定的io_context
//                    boost::asio::post(_pContexts[i], [task = std::move(task)]() {
//                        try {
//                            if (task) task();
//                        }
//                        catch (const std::exception& e) {
//                            LOGGER_ERROR(std::format("任务执行异常: {}", e.what()));
//                        }
//                        });
//
//                    // 继续接收下一个任务
//                    receiveFunc();
//                }
//            );
//            };
//
//        // 开始接收任务
//        receiveFunc();
//    }
//}
void FKIoContextThreadPool::_startTaskDispatcher()
{
    // 为每个优先级通道创建接收器
    for (size_t i = 0; i < _pTaskChannels.size(); ++i) {
        if (!_pTaskChannels[i]) continue;
        _receiveTask(i);
    }
}

void FKIoContextThreadPool::_receiveTask(size_t channelIndex)
{
    if (!_pIsRunning.load() || channelIndex >= _pTaskChannels.size() || !_pTaskChannels[channelIndex]) {
        return;
    }

    _pTaskChannels[channelIndex]->async_receive(
        [this, channelIndex](boost::system::error_code ec, std::function<void()> task) {
            if (ec) {
                if (ec != boost::asio::experimental::channel_errc::channel_closed) {
                    LOGGER_ERROR(std::format("任务接收错误: {}", ec.message()));
                }
                return;
            }            

            // 选择负载最低的io_context执行任务
            size_t contextIndex = _pNextIndex.fetch_add(1, std::memory_order_relaxed) % _pContexts.size();

            boost::asio::post(_pContexts[contextIndex], [this, task = std::move(task)]() {
                try {
                    if (task) {
                        task();
                    }
                }
                catch (const std::exception& e) {
                    LOGGER_ERROR(std::format("任务执行异常: {}", e.what()));
                }

                // 减少待处理任务计数并通知等待者
                if (_pPendingTasks.fetch_sub(1, std::memory_order_relaxed) == 1) {
                    _pCompletionCV.notify_all();
                }
                });

            // 继续接收下一个任务
            if (_pIsRunning.load()) {
                _receiveTask(channelIndex);
            }  
        }
    );
}
