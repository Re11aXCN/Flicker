/*************************************************************************************
 *
 * @ Filename	 : FKMarco.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/

#ifndef FK_MARCO_H_
#define FK_MARCO_H_

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
                                                    \
public:                                             \
    static Class* getInstance();

#define SINGLETON_CREATE_CPP(Class)  \
    std::unique_ptr<Class> Class::_instance = nullptr; \
    Class* Class::getInstance()        \
    {                                  \
        static QMutex mutex;           \
        QMutexLocker locker(&mutex);   \
        if (_instance == nullptr)      \
        {                              \
            _instance.reset(new Class());   \
        }                              \
        return _instance.get();              \
    }

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
            _instance = std::shared_ptr<Class>(new Class()); \
        });                                         \
        return _instance;                           \
    } 
#endif // !FK_MARCO_H_