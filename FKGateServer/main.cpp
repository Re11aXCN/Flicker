#include <print>
#include <json/json.h>

#include "FKServer.h"
#include  <hiredis/hiredis.h>
#include <iostream>
void TestRedis() {
	// 连接 Redis
	redisContext* c = redisConnect("127.0.0.1", 6379);
	if (c->err) {
		std::cerr << "Connect to redisServer failed: " << c->errstr << std::endl;
		redisFree(c);
		return;
	}
	std::cout << "Connect to redisServer Success" << std::endl;

	// 修复点：使用 c_str() 转换
	std::string redis_password = "123456";
	redisReply* r = (redisReply*)redisCommand(c, "auth %s", redis_password.c_str());

	if (!r) {
		std::cerr << "Redis authentication command failed" << std::endl;
		redisFree(c);
		return;
	}

	if (r->type == REDIS_REPLY_ERROR) {
		std::cerr << "Redis认证失败: " << r->str << std::endl;
	}
	else {
		std::cout << "Redis认证成功" << std::endl;
	}
	freeReplyObject(r);

	//为redis设置key
	const char* command1 = "set stest1 value1";

	//执行redis命令行
	r = (redisReply*)redisCommand(c, command1);

	//如果返回NULL则说明执行失败
	if (NULL == r)
	{
		printf("Execut command1 failure\n");
		redisFree(c);        return;
	}

	//如果执行失败则释放连接
	if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
	{
		printf("Failed to execute command[%s]\n", command1);
		freeReplyObject(r);
		redisFree(c);        return;
	}

	//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command1);

	const char* command2 = "strlen stest1";
	r = (redisReply*)redisCommand(c, command2);

	//如果返回类型不是整形 则释放连接
	if (r->type != REDIS_REPLY_INTEGER)
	{
		printf("Failed to execute command[%s]\n", command2);
		freeReplyObject(r);
		redisFree(c);        return;
	}

	//获取字符串长度
	int length = r->integer;
	freeReplyObject(r);
	printf("The length of 'stest1' is %d.\n", length);
	printf("Succeed to execute command[%s]\n", command2);

	//获取redis键值对信息
	const char* command3 = "get stest1";
	r = (redisReply*)redisCommand(c, command3);
	if (r->type != REDIS_REPLY_STRING)
	{
		printf("Failed to execute command[%s]\n", command3);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	printf("The value of 'stest1' is %s\n", r->str);
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command3);

	const char* command4 = "get stest2";
	r = (redisReply*)redisCommand(c, command4);
	if (r->type != REDIS_REPLY_NIL)
	{
		printf("Failed to execute command[%s]\n", command4);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command4);

	//释放连接资源
	redisFree(c);

}
int main(int argc, char* argv[])
{
	TestRedis();
	try
	{
		UINT16 port = 8080;
		boost::asio::io_context ioContext;
		boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
		signals.async_wait([&ioContext](const boost::system::error_code& error, int signalNumber) {
			if (error)
			{
				std::println("Error: {}", error.message());
				return;
			}
			ioContext.stop();
			});
		std::make_shared<FKServer>(ioContext, port)->start();
		ioContext.run();
	}
	catch (const std::exception& e)
	{
		std::println("EXCEPTION: {}", e.what());
		return EXIT_FAILURE;
	}
}
