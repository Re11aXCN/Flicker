#ifndef IOC_THREADPOOL_H_
#define IOC_THREADPOOL_H_

#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>

#include "universal/macros.h"

/**
 * @brief 高效的线程池类，管理多个工作线程，每个线程运行一个io_context
 * 使用单例模式确保全局唯一实例
 * 支持任务分发、负载均衡和异常处理
 */
class FKIoContextThreadPool
{
    SINGLETON_CREATE_H(FKIoContextThreadPool)
public:
    // 使用别名简化类型定义
    using ioContext = boost::asio::io_context;
    using executor = ioContext::executor_type;
    using workGuard = boost::asio::executor_work_guard<executor>;
    using taskChannel = boost::asio::experimental::channel<void(boost::system::error_code, std::function<void()>)>;
    
    // 任务优先级枚举
    enum class Priority {
        high,
        normal,
        low
    };

    /**
     * @brief 停止线程池
     */
    void stop();

    /**
     * @brief 获取下一个可用的io_context，使用轮询方式实现负载均衡
     * @return io_context引用
     */
    ioContext& getNextContext();

    /**
     * @brief 获取指定索引的io_context
     * @param index io_context的索引
     * @return io_context引用
     */
    ioContext& getContext(size_t index);
    
    /**
     * @brief 提交任务到线程池
     * @param task 要执行的任务函数
     * @param priority 任务优先级，默认为普通优先级
     * @return 是否成功提交
     */
    bool post(std::function<void()> task, Priority priority = Priority::normal);
    
    /**
     * @brief 获取线程池大小
     * @return 线程数量
     */
    size_t size() const { return _pThreads.size(); }

    /**
     * @brief 检查线程池是否正在运行
     * @return 运行状态
     */
    bool running() const { return _pIsRunning; }

    /**
    * @brief 获取当前负载信息
    * @return 负载百分比
    */
    int getCurrentLoad() const;

    /**
     * @brief 等待所有任务完成
     * @param timeoutMs 超时时间（毫秒），0表示无限等待
     * @return 是否在超时前完成
     */
    bool waitForCompletion(uint32_t timeoutMs = 0);
private:
    /**
     * @brief 初始化线程池
     * @param threadCount 线程数量，默认为系统核心数
     * @param channelCapacity 任务通道容量，默认为1024
     */
    void _initialize();

    /**
     * @brief 设置当前线程名称
     * @param name 线程名称
     */
    void _setThreadName(const std::string& name);
    
    /**
     * @brief 启动任务分发器
     */
    void _startTaskDispatcher();

    /**
    * @brief 安全地接收任务
    * @param channelIndex 通道索引
    */
    void _receiveTask(size_t channelIndex);


    FKIoContextThreadPool();
    ~FKIoContextThreadPool();
    
    std::vector<ioContext> _pContexts;                          // IO上下文数组
    std::vector<workGuard> _pGuards;                            // 工作守卫数组
    std::vector<std::unique_ptr<std::thread>> _pThreads;        // 工作线程数组
    std::vector<std::unique_ptr<taskChannel>> _pTaskChannels;   // 任务通道数组（按优先级）
    std::atomic<bool> _pIsRunning{false};                       // 运行状态标志
    std::atomic<size_t> _pNextIndex{0};                         // 下一个要使用的io_context索引

    std::atomic<size_t> _pPendingTasks{ 0 };                      // 待处理任务数量

    mutable std::mutex _pStatsMutex;                            // 统计信息互斥锁
    std::condition_variable _pCompletionCV;                     // 完成条件变量
};

#endif // !IOC_THREADPOOL_H_

