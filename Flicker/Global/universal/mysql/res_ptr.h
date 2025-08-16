#ifndef UNIVERSAL_MYSQL_RES_PTR_H_
#define UNIVERSAL_MYSQL_RES_PTR_H_
#include <memory>
#include <mysql.h>

namespace universal::mysql {
    struct ResDeleter {
        void operator()(MYSQL_RES* res) const {
            if (res) mysql_free_result(res);
        }
    };

    using ResPtr = std::unique_ptr<MYSQL_RES, ResDeleter>;
}

#endif // !UNIVERSAL_MYSQL_RES_PTR_H_