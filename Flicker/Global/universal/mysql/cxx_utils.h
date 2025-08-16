#ifndef UNIVERSAL_MYSQL_CXX_UTILS_H_
#define UNIVERSAL_MYSQL_CXX_UTILS_H_
#include <type_traits>
#include <optional>
#include <tuple>
#include <string>
#include <stdexcept>
#include <chrono>
#include <expected>
#include <mysql.h>
#include <mysqld_error.h>
#include "universal/utils.h"
namespace universal::mysql {
    enum class DbOperatorStatus {
        Success,
        DataAlreadyExist,
        DataNotExist,
        DatabaseError,
        InvalidParameters
    };

    // MySQL连接错误类型
    enum class MySQLErrorCode {
        Success,
        InitializationFailed,
        ConnectionFailed,
        ConnectionClosed,
        ConnectionTimeout,
        InvalidParameters,
        PrepareStatementFailed,
        ExecuteStatementFailed,
        BindParameterFailed,
        FetchResultFailed,
        NoMetadataAvailable,
        BindResultFailed,
        StoreResultFailed,
        QueryFailed,
        TransactionFailed,
        UnsupportedOperation,
        OutOfMemory,
        PermissionDenied,
        DatabaseNotFound,
        TableNotFound,
        FieldNotFound,
        DuplicateEntry,
        UnknownError
    };

    // MySQL错误信息结构
    struct MySQLError {
        MySQLErrorCode code;
        std::string message;
        int mysql_errno = 0;
        
        MySQLError(MySQLErrorCode c, const std::string& msg, int err_no = 0)
            : code(c), message(msg), mysql_errno(err_no) {}
        
        friend std::ostream& operator<<(std::ostream& os, const MySQLError& error) {
            os << "MySQLError[" << static_cast<int>(error.code) << "]: " << error.message;
            if (error.mysql_errno != 0) {
                os << " (MySQL errno: " << error.mysql_errno << ")";
            }
            return os;
        }
    };

    // 将MySQL错误码转换为MySQLErrorCode
    inline MySQLErrorCode mapMySQLError(int mysql_errno) {
        switch (mysql_errno) {
            case 0: return MySQLErrorCode::Success;
            case ER_ACCESS_DENIED_ERROR: return MySQLErrorCode::PermissionDenied;
            case ER_BAD_DB_ERROR: return MySQLErrorCode::DatabaseNotFound;
            case ER_BAD_TABLE_ERROR: return MySQLErrorCode::TableNotFound;
            case ER_BAD_FIELD_ERROR: return MySQLErrorCode::FieldNotFound;
            case ER_DUP_ENTRY: return MySQLErrorCode::DuplicateEntry;
            case ER_OUTOFMEMORY: return MySQLErrorCode::OutOfMemory;
            case ER_CON_COUNT_ERROR:
            case ER_OUT_OF_RESOURCES: return MySQLErrorCode::ConnectionFailed;
            default: return MySQLErrorCode::UnknownError;
        }
    }

    // std::expected类型别名
    template<typename T>
    using MySQLResult = std::expected<T, MySQLError>;
    
    // 常用的结果类型
    using MySQLVoidResult = MySQLResult<void>;
    using MySQLBoolResult = MySQLResult<bool>;
    using MySQLSizeResult = MySQLResult<size_t>;
    using MySQLStringResult = MySQLResult<std::string>;
    
    // 辅助函数：创建成功结果
    template<typename T>
    inline MySQLResult<T> makeSuccess(T&& value) {
        return MySQLResult<T>(std::forward<T>(value));
    }
    
    inline MySQLVoidResult makeSuccess() {
        return MySQLVoidResult();
    }
    
    // 辅助函数：创建错误结果
    template<typename T>
    inline MySQLResult<T> makeError(MySQLErrorCode code, const std::string& message, int mysql_errno = 0) {
        return std::unexpected(MySQLError(code, message, mysql_errno));
    }

    inline MySQLVoidResult makeError(MySQLErrorCode code, const std::string& message, int mysql_errno = 0) {
        return std::unexpected(MySQLError(code, message, mysql_errno));
    }
    
    // 辅助函数：从MySQL错误创建结果
    template<typename T>
    inline MySQLResult<T> makeErrorFromMySQL(MYSQL* mysql, MySQLErrorCode code, const std::string& context = "") {
        int err_no = mysql_errno(mysql);
        std::string msg = mysql_error(mysql);
        if (!context.empty()) {
            msg = context + ": " + msg;
        }
        return std::unexpected(MySQLError(code, msg, err_no));
    }

    inline MySQLVoidResult makeErrorFromMySQL(MYSQL* mysql, MySQLErrorCode code, const std::string& context = "") {
        int err_no = mysql_errno(mysql);
        std::string msg = mysql_error(mysql);
        if (!context.empty()) {
            msg = context + ": " + msg;
        }
        mysql_close(mysql);
        mysql = nullptr;
        return std::unexpected(MySQLError(code, msg, err_no));
    }
    
    // 辅助函数：从MySQL STMT错误创建结果
    template<typename T>
    inline MySQLResult<T> makeErrorFromStmt(MYSQL_STMT* stmt, MySQLErrorCode code, const std::string& context = "") {
        int err_no = mysql_stmt_errno(stmt);
        std::string msg = mysql_stmt_error(stmt);
        if (!context.empty()) {
            msg = context + ": " + msg;
        }
        return std::unexpected(MySQLError(code, msg, err_no));
    }

    inline MySQLVoidResult makeErrorFromStmt(MYSQL_STMT* stmt, MySQLErrorCode code, const std::string& context = "") {
        int err_no = mysql_stmt_errno(stmt);
        std::string msg = mysql_stmt_error(stmt);
        if (!context.empty()) {
            msg = context + ": " + msg;
        }
        return std::unexpected(MySQLError(code, msg, err_no));
    }


    //< Mysql query result set sorting and pagination,
    //  {SortOrder} enum       : ascending and descending,
    //  {Pagination} pagination: limit, offset,
    //  {fixed_string}         : sorting according to defined string targets
    // 
    //  "SELECT * FROM table_name ORDER BY <fixed_string> <SortOrder.Ascending/Descending> LIMIT <Pagination.limit> OFFSET <Pagination.offset>";
    //  "SELECT * FROM table_name ORDER BY field_name ASC/DESC LIMIT offset, limit";
    enum class SortOrder {
        Ascending,  // 升序
        Descending  // 降序
    };

    struct Pagination {
        size_t limit;
        size_t offset;
    };

    template<size_t N>
    struct OrderBy {
        SortOrder order;
        utils::string::fixed_string<N + 1> fieldName; // +1 是为了包含'\0' 结尾符

        auto operator<=>(const OrderBy&) const = default;
    };

    struct mysql_char { // stmt bind string type
        const char* val;
        unsigned long len;
        mysql_char(const char* v, unsigned long l) : val(v), len(l) {}
        ~mysql_char() {}
        mysql_char(const mysql_char&) = default;
        mysql_char& operator=(const mysql_char&) = default;
        mysql_char(mysql_char&&) = default;
        mysql_char& operator=(mysql_char&&) = default;
    };

    struct mysql_varchar { // stmt bind string type
        const char* val;
        unsigned long len;
        mysql_varchar(const char* v, unsigned long l) : val(v), len(l) {}
        ~mysql_varchar() {}
        mysql_varchar(const mysql_varchar&) = default;
        mysql_varchar& operator=(const mysql_varchar&) = default;
        mysql_varchar(mysql_varchar&&) = default;
        mysql_varchar& operator=(mysql_varchar&&) = default;
    };

    /**
    * @brief 用于 updateFieldsByCondition(<condition>, <fields>, <values>) 参数包输入限制
    * @example
    *
    * "password" in MYSQL type is "VARCHAR(60)", "update_time" in MYSQL type is "TIMESTAMP(3)"
    * updateFieldsByCondition(
    *    condition,
    *    std::vector<std::string>{"password", "update_time"},
    *    BindableParam{ mysql_varchar{"updated_password", 16} },)
    *    ExpressionParam{ "NOW(3)" })
    *
    * @like "UPDATE table SET password = <bindable_param>, update_time = NOW(3) WHERE <condition>"
    *
    * @note <mysql_varchar> see "Inl-ValueBinder.ipp" <condition> see "Inl-QueryCondition.ipp"
    */
    // 可绑定参数的包装
    template<typename T>
    struct BindableParam {
        T value;
        explicit BindableParam(T&& v) : value(std::forward<T>(v)) {}
        ~BindableParam() = default;
    };

    // 表达式参数的包装（不需要绑定）
    struct ExpressionParam {
        std::string expr;
        explicit ExpressionParam(const char* e) : expr(e) {}
        explicit ExpressionParam(const std::string& e) : expr(e) {}
        explicit ExpressionParam(std::string&& e) : expr(std::move(e)) {}
        ~ExpressionParam() = default;
    };

    template<typename Type>
    struct is_mysql_varchar_type : std::false_type {};
    template<>
    struct is_mysql_varchar_type<mysql_varchar> : std::true_type {};
    template<typename Type>
    inline constexpr bool is_mysql_varchar_type_v = is_mysql_varchar_type<Type>::value;

    template<typename Type>
    struct is_mysql_char_type : std::false_type {};
    template<>
    struct is_mysql_char_type<mysql_char> : std::true_type {};
    template<typename Type>
    inline constexpr bool is_mysql_char_type_v = is_mysql_char_type<Type>::value;

    template<typename T>
    struct is_bindable_param : std::false_type {};
    template<typename T>
    struct is_bindable_param<BindableParam<T>> : std::true_type {};
    template<typename T>
    struct is_expression_param : std::false_type {};
    template<>
    struct is_expression_param<ExpressionParam> : std::true_type {};
    template<typename T>
    constexpr bool is_supported_param_v = is_bindable_param<std::decay_t<T>>::value ||
        is_expression_param<std::decay_t<T>>::value;

    template<typename Type>
    struct is_optional_type : std::false_type {};
    template<typename Type>
    struct is_optional_type<std::optional<Type>> : std::true_type {};
    template<typename Type>
    inline constexpr bool is_optional_type_v = is_optional_type<Type>::value;

    // 参数包分离器，将BindableParam和ExpressionParam分离
    template<typename...>
    struct Separator;
    // 终止特化：无参数
    template<>
    struct Separator<> {
        static std::pair<std::tuple<>, std::tuple<>> separate() {
            return { {}, {} };
        }
    };

    // 递归特化：至少一个参数
    template<typename First, typename... Rest>
    struct Separator<First, Rest...> {
        // 检查参数类型
        static_assert(is_supported_param_v<First>,
            "Unsupported parameter type. Use BindableParam or ExpressionParam");

        // 递归处理剩余参数
        static auto separate(First&& first, Rest&&... rest) {
            auto [bindables, expressions] = Separator<Rest...>::separate(
                std::forward<Rest>(rest)...
            );

            if constexpr (is_bindable_param<std::decay_t<First>>::value) {
                return std::make_pair(
                    std::tuple_cat(std::make_tuple(std::forward<First>(first)), bindables),
                    expressions
                );
            }
            else {
                return std::make_pair(
                    bindables,
                    std::tuple_cat(std::make_tuple(std::forward<First>(first)), expressions)
                );
            }
        }
    };

    //< MYSQL_STMT preprocess the binding, and use {ValueBinder} - the collapsed expression
    //  to expand the specific <SET/WHERE> condition parameter type of the binding
    // 
    //< If the query field type is a string, use the {mysql_varchar} struct 
    //  as a parameter pass instead of std::string
    // 单个值绑定
    template<typename ValueType>
    inline void bindSingleValue(MYSQL_BIND& bind, ValueType&& value)
    {
        using RawType = std::remove_cv_t<std::remove_reference_t<ValueType>>;
        //using Type = std::decay_t<ValueType>; // 类型退化、数组→指针转换、函数→函数指针
        if constexpr (std::is_integral_v<RawType>) {
            if constexpr (std::is_same_v<RawType, bool>) {
                bind.buffer_type = MYSQL_TYPE_TINY;
            }
            else if constexpr (sizeof(RawType) <= 1) {
                bind.buffer_type = MYSQL_TYPE_TINY;
            }
            else if constexpr (sizeof(RawType) <= 2) {
                bind.buffer_type = MYSQL_TYPE_SHORT;
            }
            else if constexpr (sizeof(RawType) <= 4) {
                bind.buffer_type = MYSQL_TYPE_LONG;
            }
            else {
                bind.buffer_type = MYSQL_TYPE_LONGLONG;
            }

            bind.is_unsigned = std::is_unsigned_v<RawType>;
            bind.buffer = const_cast<RawType*>(&value);
            bind.buffer_length = sizeof(value);
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (std::is_floating_point_v<RawType>) {
            if constexpr (std::is_same_v<RawType, float>) {
                bind.buffer_type = MYSQL_TYPE_FLOAT;
            }
            else {
                bind.buffer_type = MYSQL_TYPE_DOUBLE;
            }

            bind.buffer = const_cast<RawType*>(&value);
            bind.buffer_length = sizeof(value);
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (is_mysql_varchar_type_v<RawType>) {
            bind.buffer_type = MYSQL_TYPE_VAR_STRING;
            bind.buffer = const_cast<char*>(value.val);
            bind.buffer_length = value.len + 1;
            bind.length = const_cast<unsigned long*>(&value.len);// &value.len;
            bind.is_null = 0;
        }
        else if constexpr (is_mysql_char_type_v<RawType>) {
            bind.buffer_type = MYSQL_TYPE_STRING;
            bind.buffer = const_cast<char*>(value.val);
            bind.buffer_length = value.len + 1;
            bind.length = const_cast<unsigned long*>(&value.len);// &value.len;
            bind.is_null = 0;
        }
        else if constexpr (std::is_same_v<RawType, MYSQL_TIME>) {
            bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
            bind.buffer = const_cast<MYSQL_TIME*>(&value);
            bind.buffer_length = sizeof(value);
            bind.is_null = 0;
            bind.length = 0;
        }
        else if constexpr (is_optional_type_v<RawType>) {
            if (value.has_value()) {
                bindSingleValue(bind, *value);
            }
            else {
                bind.buffer_type = MYSQL_TYPE_NULL;
                bind.is_null = 1;
            }
        }
        else {
            throw std::runtime_error("Unsupported value type in bindSingleValue");
        }
    }
    
    // 使用折叠表达式替代递归
    template<typename... Args>
    inline void bindParams(MYSQL_BIND* binds, Args&&... args) {
        size_t index = 0;
        (bindSingleValue(binds[index++], std::forward<Args>(args)), ...);
    }
}

#endif // !UNIVERSAL_MYSQL_CXX_UTILS_H_