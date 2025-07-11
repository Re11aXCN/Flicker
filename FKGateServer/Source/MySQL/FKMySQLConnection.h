/*************************************************************************************
 *
 * @ Filename	 : FKMySQLConnection.h
 * @ Description : MySQL连接封装类，使用纯C API
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/22
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_MYSQL_CONNECTION_H_
#define FK_MYSQL_CONNECTION_H_

#include <string>
#include <chrono>
#include <mutex>
#include <mysql.h>

// MySQL连接包装类，包含连接和最后活动时间
class FKMySQLConnection {
public:
	// 构造函数，接收一个已初始化的MYSQL指针
	FKMySQLConnection(MYSQL* mysql);
	
	// 析构函数，负责关闭连接
	~FKMySQLConnection();

	// 获取原始MYSQL连接指针
	MYSQL* getMysql();
	
	// 更新最后活动时间
	void updateActiveTime();
	
	// 获取最后活动时间
	std::chrono::steady_clock::time_point getLastActiveTime() const;
	
	// 检查连接是否已过期
	bool isExpired(std::chrono::milliseconds timeout) const;
	
	// 执行简单查询
	bool executeQuery(const std::string& query);
	
	// 开始事务
	bool startTransaction();
	
	// 提交事务
	bool commit();
	
	// 回滚事务
	bool rollback();

	// 检查连接是否有效
	bool isValid();

private:
	// MySQL连接指针
	MYSQL* _mysql;
	
	// 最后活动时间
	std::chrono::steady_clock::time_point _lastActiveTime;
	
	// 互斥锁，保证线程安全
	mutable std::mutex _mutex;
};

#endif // !FK_MYSQL_CONNECTION_H_