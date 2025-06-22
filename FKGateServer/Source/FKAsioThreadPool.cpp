#include "FKAsioThreadPool.h"
#include "FKServerConfig.h"
#include <print>

SINGLETON_CREATE_CPP(FKAsioThreadPool)
FKAsioThreadPool::FKAsioThreadPool()
	: _pIsRunning(false)
	, _pNextIndex(0)
{
	std::println("线程池构造");
	
	// 从配置加载参数
	FKServerConfig* config = FKServerConfig::getInstance();
	size_t threadCount = config->getAsioThreadPoolConfig().threadCount;
	size_t channelCapacity = config->getAsioThreadPoolConfig().channelCapacity;
	
	// 使用配置的参数初始化线程池
	_initialize(threadCount, channelCapacity);
}

FKAsioThreadPool::~FKAsioThreadPool()
{
	stop();
}

void FKAsioThreadPool::_initialize(size_t threadCount /*= std::thread::hardware_concurrency()*/, size_t channelCapacity /*= 1024*/)
{
	if (_pIsRunning) return;
	std::println("线程池初始化");

	_pIsRunning = true;
	std::vector<ioContext> newContexts{ threadCount };
	_pContexts.swap(newContexts);
	_pGuards.clear();
	_pGuards.reserve(threadCount);
	_pThreads.resize(threadCount);

	// 创建任务通道
	_pTaskChannels.resize(3); // 高、中、低三个优先级
	for (auto& channel : _pTaskChannels) {
		channel = std::make_unique<taskChannel>(_pContexts[0], channelCapacity);
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
			}
			catch (const std::exception& e) {
				std::println("线程池异常: {}", e.what());
				_logError("线程池运行异常", e.what(), std::source_location::current());
			}
			});
	}

	// 启动任务分发器
	_startTaskDispatcher();
}

void FKAsioThreadPool::stop()
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

FKAsioThreadPool::ioContext& FKAsioThreadPool::getNextContext()
{
	if (_pContexts.empty()) {
		throw std::runtime_error("线程池未初始化");
	}
	// 使用原子操作实现线程安全的轮询
	size_t current = _pNextIndex.fetch_add(1, std::memory_order_relaxed);
	return _pContexts[current % _pContexts.size()];
}

FKAsioThreadPool::ioContext& FKAsioThreadPool::getContext(size_t index)
{
	return _pContexts[index % _pContexts.size()];
}

bool FKAsioThreadPool::post(std::function<void()> task, Priority priority /*= Priority::normal*/)
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
				std::println("任务提交失败: {}", ec.message());
			}
		}
	);

	return true;
}

void FKAsioThreadPool::_setThreadName(const std::string& name)
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

void FKAsioThreadPool::_startTaskDispatcher()
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
							std::println("任务接收错误: {}", ec.message());
						}
						return;
					}

					// 直接使用当前通道绑定的io_context
					boost::asio::post(_pContexts[i], [task = std::move(task)]() {
						try {
							if (task) task();
						}
						catch (const std::exception& e) {
							std::println("任务执行异常: {}", e.what());
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

void FKAsioThreadPool::_logError(std::string_view message, std::string_view details, const std::source_location& location)
{
	std::println("[错误] {} - {} (文件: {}, 行: {}, 函数: {})\n",
		message, details, location.file_name(), location.line(), location.function_name());
}
