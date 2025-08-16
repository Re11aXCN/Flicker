#include "bcrypt.h"
#include <iostream>
int main()
{
    std::string hash = bcrypt::generateHash("123456");
    bool ok = bcrypt::validatePassword("123456", hash);
    if (ok) {
        std::cout << "Password is correct." << std::endl;
    }
    else {
        std::cout << "Password is incorrect." << std::endl;
    }
    return 0;
}