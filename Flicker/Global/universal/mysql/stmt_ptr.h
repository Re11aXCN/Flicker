#ifndef UNIVERSAL_MYSQL_STMT_PTR_H_
#define UNIVERSAL_MYSQL_STMT_PTR_H_
#include <memory>
#include <mysql.h>

namespace universal::mysql {
    struct StmtDeleter {
        void operator()(MYSQL_STMT* stmt) const {
            if (stmt) mysql_stmt_close(stmt);
        }
    };

    using StmtPtr = std::unique_ptr<MYSQL_STMT, StmtDeleter>;
}

#endif // !UNIVERSAL_MYSQL_STMT_PTR_H_