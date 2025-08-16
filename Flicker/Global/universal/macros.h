#ifndef UNIVERSAL_MACROS_H_
#define UNIVERSAL_MACROS_H_
#include <cstdint>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

//短整形高低字节交换
#define Swap16(A) ((((std::uint16_t)(A) & 0xff00) >> 8) | (((std::uint16_t)(A) & 0x00ff) << 8))
//长整形高低字节交换
#define Swap32(A) ((((std::uint32_t)(A) & 0xff000000) >> 24) | \
            (((std::uint32_t)(A) & 0x00ff0000) >>  8) | \
            (((std::uint32_t)(A) & 0x0000ff00) <<  8) | \
            (((std::uint32_t)(A) & 0x000000ff) << 24))

#define Swap64(A) ((((std::uint64_t)(A) & 0xff00000000000000ULL) >> 56) | \
                (((std::uint64_t)(A) & 0x00ff000000000000ULL) >> 40) | \
                (((std::uint64_t)(A) & 0x0000ff0000000000ULL) >> 24) | \
                (((std::uint64_t)(A) & 0x000000ff00000000ULL) >> 8)  | \
                (((std::uint64_t)(A) & 0x00000000ff000000ULL) << 8)  | \
                (((std::uint64_t)(A) & 0x0000000000ff0000ULL) << 24) | \
                (((std::uint64_t)(A) & 0x000000000000ff00ULL) << 40) | \
                (((std::uint64_t)(A) & 0x00000000000000ffULL) << 56))

#define M_PROPERTY_CREATE(TYPE, M)                          \
public:                                                     \
    void set##M(TYPE M)                                     \
    {                                                       \
        _p##M = M;                                          \
    }                                                       \
    TYPE get##M() const                                     \
    {                                                       \
        return _p##M;                                       \
    }                                                       \
                                                            \
private:                                                    \
    TYPE _p##M;

#define SINGLETON_CREATE_H(Class)                           \
private:                                                    \
    static std::unique_ptr<Class> _instance;                \
    friend std::default_delete<Class>;                      \
    template<typename... Args>                              \
    static std::unique_ptr<Class> _create(Args&&... args) { \
        struct make_unique_helper : public Class            \
        {                                                   \
            make_unique_helper(Args&&... a) : Class(std::forward<Args>(a)...){}     \
        };                                                                          \
        return std::make_unique<make_unique_helper>(std::forward<Args>(args)...);   \
    }                                                       \
public:                                                     \
    Class(const Class &) = delete;                          \
    Class &operator=(const Class &) = delete;               \
    Class(Class &&) = delete;                               \
    Class &operator=(Class &&) = delete;                    \
    static Class* getInstance();

#define SINGLETON_CREATE_CPP(Class)                     \
    std::unique_ptr<Class> Class::_instance = nullptr;  \
    Class* Class::getInstance()                 \
    {                                           \
        static std::once_flag flag;             \
        std::call_once(flag, [&]() {            \
            _instance = _create();              \
        });                                     \
        return _instance.get();                 \
    }

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
    Class(const Class &) = delete;                          \
    Class &operator=(const Class &) = delete;               \
    Class(Class &&) = delete;                               \
    Class &operator=(Class &&) = delete;                    \
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
#endif // UNIVERSAL_MACROS_H_