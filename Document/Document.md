# 项目说明

* NexUs
* NexUsExample
* Flicker
* FKGateServer



NexUs和NexUsExample是开源项目[ElaWidgetsTool](https://github.com/Liniyous/ElaWidgetTools)自定义模块主要用来给<u>Flicker客户端</u>进行UI设计实现

Flicker客户端

FKGateServer服务器进行处理客户端发送的消息

# 开发记录

## 1. 项目架构

`FKLoginWidget` 是客户端登录UI并负责处理业务，`FKHttpManager`将客户端 点击控件 产生的请求 发送封装的JSON消息 给`FKGateServer`服务器，服务器则是通过`boost`和`grpc`进行处理请求并响应，具体为`FKServer`监听端口，`async_accept`收到请求后通过`FKHttpConnection`作为一个通道`async_read`请求后，将具体业务处理交给`FKLogicSystem`单例类回调我们提供的已经注册的业务方法，最后`FKHttpConnection`将响应处理的消息交给客户端解析处理，展示给用户

## 2. 测试记录

打开服务器我们进行post测试

![image-20250619002142967](assets/image-20250619002142967.png)

6/19日提交测试

![image-20250619210431088](assets/image-20250619210431088.png)

![image-20250619002118118](assets/image-20250619002118118.png)

![image-20250619002231101](assets/image-20250619002231101.png)

## 3. TODO

```
密码加密，建议采用雪花算法生成唯一值sql存储然后进行hash，自定义打乱顺序，使用这个乱序作为密钥aes加密，解密的时候将hash根据规定的打乱顺序进行打乱得到密钥解密

登录失败五次后锁定账号1个小时不能登录

注册到数据库

重置密码，需要传入username、email来进行判断是否存在用户，存在就重置密码，qt发送数据，服务器进行判断

忘记密码，初始给用户名或者邮箱，一个验证码lineedit，验证成功之后切换页面输入密码进行修改输入密码页面5分钟有效（可以根据邮箱验证码有效实际？）

```

## 4. 问题记录

### shared_ptr管理的单例

```cpp
#define SINGLETON_CREATE_SHARED_H(Class)          \
private:                                            \
    static std::shared_ptr<Class> _instance;        \
    friend struct std::default_delete<Class>;       \
public:                                             \
    static std::shared_ptr<Class> getInstance();

#define SINGLETON_CREATE_SHARED_CPP(Class)        \
    std::shared_ptr<Class> Class::_instance = nullptr; \
    std::shared_ptr<Class> Class::getInstance()     \
    {                                               \
        static std::once_flag flag;                 \
        std::call_once(flag, [&]() {                \
            _instance = std::shared_ptr<Class>(new Class());   \
        });                                         \
        return _instance;                           \
    } 

// .h
class FKLogicSystem {
	SINGLETON_CREATE_SHARED_H(FKLogicSystem)
private:
	FKLogicSystem();
	~FKLogicSystem() = default;
};

// .cpp
SINGLETON_CREATE_SHARED_CPP(FKLogicSystem)
/* 宏展开
std::shared_ptr<FKLogicSystem> FKLogicSystem::_instance = nullptr;
std::shared_ptr<FKLogicSystem> FKLogicSystem::getInstance() {
	static std::once_flag flag;
    std::call_once(flag, [&]() {
    _instance = std::shared_ptr<FKLogicSystem>(new FKLogicSystem());
    });
    return _instance;
}
*/
MSVC编译器报错
没有与参数列表匹配的构造函数 "std::shared_ptr<_Ty>::shared_ptr [其中 _Ty=FKLogicSystem]" 实例参数类型为FKLogicSystem*；
    
如果将 ~FKLogicSystem() = default; 设置为 public 就没有问题， 推测是shared_ptr访问不到析构函数，即使设置了friend struct std::default_delete<Class>;也没用
    
    
// 修复，手动指定析构
#define SINGLETON_CREATE_SHARED_H(Class)          \
private:                                            \
    static std::shared_ptr<Class> _instance;        \
public:                                             \
    static std::shared_ptr<Class> getInstance();

#define SINGLETON_CREATE_SHARED_CPP(Class)        \
    std::shared_ptr<Class> Class::_instance = nullptr; \
    std::shared_ptr<Class> Class::getInstance()     \
    {                                               \
        static std::once_flag flag;                 \
        std::call_once(flag, [&]() {                \
            _instance = std::shared_ptr<Class>(new Class(), [](Class* ptr) { delete ptr; });   \
        });                                         \
        return _instance;                           \
    } 
或者
#define SINGLETON_CREATE_SHARED_H(Class)                    \
private:                                                    \
    static std::shared_ptr<Class> _instance;                \
    friend std::default_delete<Class>;                      \
    template<typename... Args>                              \
    static std::shared_ptr<Class> _create(Args&&... args) { \
        struct make_shared_helper : public Class            \
        {                                                   \
            make_shared_helper(Args&&... a) : Class(std::forward<Args>(a)...){}     \
        };                                                  \
        return std::make_shared<make_shared_helper>(std::forward<Args>(args)...);   \
    }                                                       \
public:                                                     \
    static std::shared_ptr<Class> getInstance();

// std::shared_ptr<Class>(new Class(), [](Class* ptr) { delete ptr; });   
#define SINGLETON_CREATE_SHARED_CPP(Class)          \
    std::shared_ptr<Class> Class::_instance = nullptr; \
    std::shared_ptr<Class> Class::getInstance()     \
    {                                               \
        static std::once_flag flag;                 \
        std::call_once(flag, [&]() {                \
            _instance = _create();                  \
        });                                         \
        return _instance;                           \
    } 
```

### redis测试

redis是C语言写的，如果使用std::string，即使密码正确`redisReply* r = (redisReply*)redisCommand(c, "auth %s", redis_password);`仍会得到的是响应错误，

原因分析：

1. 在C++中，`std::string`类型**并不是以空字符('\0')结尾的字符串**（虽然实际上C++11标准保证std::string内部存储是连续的，并且可以通过c_str()获取空字符结尾的字符串）。但是，`redisCommand`函数是一个可变参数函数，类似于`printf`，它期望一个格式字符串和一系列参数。当使用`%s`格式说明符时，它期望一个**以空字符结尾的字符串（即C风格字符串）**。

```C
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

```

### boost:asio接收中文乱码

```c++
QJsonObject requestObj;
requestObj["request_type"] = static_cast<int>(Http::RequestSeviceType::GET_VARIFY_CODE);
requestObj["message"] = "Client request for varify code"; // 请求获取验证码

// 创建数据对象
QJsonObject dataObj;
dataObj["email"] = email;
requestObj["data"] = dataObj;  // 将数据对象添加到请求中
```

如果qt发送的json包含了中文消息，那么GateServer使用boost解析的时候将会产生乱码，并且

```c++
		std::string body = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::println("Received Body: \n{}", body);
```

读取得到的字符串是未知的，`catch`到异常，所以在通信的时候还是采用英文传输

即使qt设置了请求为utf-8也没有用，可能是boost底层实现和std::string实现不支持中文的原因

```c++
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
```

![image-20250622230904247](assets/image-20250622230904247.png)

正确能够响应逻辑

![image-20250622230704404](assets/image-20250622230704404.png)

### QPropertyAnimation

QParallelAnimationGroup并行执行

QSequentialAnimationGroup顺序执行

QPauseAnimation只能用于QSequentialAnimationGroup进行暂停，如果QParallelAnimationGroup需要进行间隔建议使用关键帧		setKeyValueAt(0.44, 0.3);

比如 A动画1.25s，B动画需要再A动画执行0.55s后才开始，并且需要并行执行，那么B需要设置动画为1.25s，然后B->setKeyValueAt(0.44, 0.3)// 0.44对于1.25s的比例是0.55s，0.3是setStartValue的初始值

### QPainter::CompositionMode

测试颜色为 红(1.0, 0.0, 0.0, 0.7) 和 黑(0.0, 0.0, 0.0, 0.7)，

CompositionMode_SourceIn 源图颜色有透明度

![image-20250704043107596](assets/image-20250704043107596.png)

CompositionMode_Source 源图颜色无透明度

![image-20250704043125535](assets/image-20250704043125535.png)

CompositionMode_SourceAtop + CompositionMode_Clear 目标图像和源图像进行应用透明度混合

![image-20250704043148594](assets/image-20250704043148594.png)
