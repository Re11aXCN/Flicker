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
[GateServer]
Port = 8080
[VarifyServer]
Port = 50051


考虑使用上述ini配置

或者使用json/xml进行存储这样的数据，然后写一个class 工具类进行解析读取消息，给Filcker和FKGateServer进行设置参数
```

## 4. 问题记录

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

