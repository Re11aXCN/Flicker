#ifndef FK_ASIO_THREADPOOL_H_
#define FK_ASIO_THREADPOOL_H_

#include <vector>
#include <boost/asio.hpp>

#include "FKMarco.h"

/**
 * @brief 线程池类，管理多个工作线程，每个线程运行一个io_context
 * 使用单例模式确保全局唯一实例
 */
class FKAsioThreadPool
{
	SINGLETON_CREATE_SHARED_H(FKAsioThreadPool)
public:
	// 使用别名简化工作守卫类型
	using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
private:
	FKAsioThreadPool();
};

#endif // !FK_ASIO_THREADPOOL_H_

