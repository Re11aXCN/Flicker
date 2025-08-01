/*************************************************************************************
 *
 * @ Filename     : macro.h
 * @ Description :
 *
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By:
 * Modifications:
 * ======================================
*************************************************************************************/

#ifndef MARCO_H_
#define MARCO_H_
#include <memory>
#include <magic_enum/magic_enum_all.hpp>
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

#define SINGLETON_CREATE_H(Class)                 \
private:                                            \
    static std::unique_ptr<Class> _instance;        \
    friend std::default_delete<Class>;              \
    template<typename... Args>                              \
    static std::unique_ptr<Class> _create(Args&&... args) { \
        struct make_unique_helper : public Class            \
        {                                                   \
            make_unique_helper(Args&&... a) : Class(std::forward<Args>(a)...){}     \
        };                                                  \
        return std::make_unique<make_unique_helper>(std::forward<Args>(args)...);   \
    }                                                       \
public:                                             \
    static Class* getInstance();

#define SINGLETON_CREATE_CPP(Class)  \
    std::unique_ptr<Class> Class::_instance = nullptr; \
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
#endif // !MARCO_H_