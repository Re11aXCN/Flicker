#ifndef BCRYPT_H
#define BCRYPT_H

#include <string>

#ifndef BCRYPT_EXPORT
#define BCRYPT_API __declspec(dllimport)
#else
#define BCRYPT_API __declspec(dllexport)
#endif

namespace bcrypt {

    BCRYPT_API std::string generateHash(const std::string & password , unsigned rounds = 10);

    BCRYPT_API bool validatePassword(const std::string & password, const std::string & hash);

}

#endif // BCRYPT_H
