// MySQL语句智能指针，用于自动管理MYSQL_STMT资源
class MySQLStmtPtr {
private:
    MYSQL_STMT* _stmt;
    bool _released;

public:
    explicit MySQLStmtPtr(MYSQL_STMT* stmt) : _stmt(stmt), _released(false) {}

    ~MySQLStmtPtr() {
        release();
    }

    // 禁止复制
    MySQLStmtPtr(const MySQLStmtPtr&) = delete;
    MySQLStmtPtr& operator=(const MySQLStmtPtr&) = delete;

    // 允许移动
    MySQLStmtPtr(MySQLStmtPtr&& other) noexcept : _stmt(other._stmt), _released(other._released) {
        other._stmt = nullptr;
        other._released = true;
    }

    MySQLStmtPtr& operator=(MySQLStmtPtr&& other) noexcept {
        if (this != &other) {
            release();
            _stmt = other._stmt;
            _released = other._released;
            other._stmt = nullptr;
            other._released = true;
        }
        return *this;
    }

    // 获取原始指针
    MYSQL_STMT* get() const {
        return _stmt;
    }

    // 释放资源
    void release() {
        if (_stmt && !_released) {
            mysql_stmt_close(_stmt);
            _released = true;
        }
        _stmt = nullptr;
    }

    // 转换为原始指针
    operator MYSQL_STMT* () const {
        return _stmt;
    }

    // 检查是否有效
    bool isValid() const {
        return _stmt != nullptr && !_released;
    }

    // 释放所有权（不关闭语句）
    MYSQL_STMT* release_ownership() {
        MYSQL_STMT* temp = _stmt;
        _stmt = nullptr;
        _released = true;
        return temp;
    }
};

// 数据库异常类
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message) : std::runtime_error(message) {}
};

// 结果集解析器，将内存中的数据转换为C++具体类型
class ResultSetParser {
public:
    // 从MYSQL_BIND获取值的方法
    template<typename T>
    std::optional<T> getValue(MYSQL_BIND* bind, unsigned long length, bool isNull, enum_field_types field_type) const {
        return getValueImpl(bind, length, isNull, field_type, type_tag<T>{});
    }

    // 从MYSQL_ROW获取值的方法
    template<typename T>
    std::optional<T> getValue(MYSQL_ROW row, unsigned int fieldIndex, unsigned long* lengths, bool isNull, enum_field_types field_type) const {
        return getValueFromRowImpl(row, fieldIndex, lengths, isNull, field_type, type_tag<T>{});
    }

    // 检查字段是否为NULL
    static bool isFieldNull(MYSQL_ROW row, unsigned int fieldIndex) {
        return row[fieldIndex] == nullptr;
    }

protected:
    template<typename T>
    struct type_tag {};

    // MYSQL_BIND类型转换分发
    template<typename T>
    std::optional<T> getValueImpl(MYSQL_BIND* bind, unsigned long length, bool isNull, enum_field_types field_type, type_tag<T>) const {
        if (isNull) {
            return std::nullopt;
        }

        // 处理 optional 类型（非 NULL 情况）
        /*if constexpr (is_optional_type_v<T>) {
            using ValueType = typename T::value_type;
            return getValueImpl<ValueType>(bind, length, false, type_tag<ValueType>{});
        }*/
        // 处理算术类型
        if constexpr (std::is_arithmetic_v<T>) {
            return *reinterpret_cast<T*>(bind->buffer);
        }
        // 处理字符串类型
        else if constexpr (std::is_same_v<T, std::string>) {
            return std::string(static_cast<const char*>(bind->buffer), length);
        }
        // 处理时间点类型
        else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            if (field_type == MYSQL_TYPE_TIMESTAMP || field_type == MYSQL_TYPE_DATETIME)
                return MysqlTimeUtils::mysqlTimeToChrono(*static_cast<MYSQL_TIME*>(bind->buffer));
            else if (field_type == MYSQL_TYPE_DOUBLE || field_type == MYSQL_TYPE_NEWDECIMAL || field_type == MYSQL_TYPE_FLOAT || field_type == MYSQL_TYPE_DECIMAL)
                return std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(static_cast<uint64_t>(*static_cast<double*>(bind->buffer)))
                );
            else
                throw DatabaseException("Unsupported field type for time_point conversion " __FUNCTION__ "");
        }
        // 不支持的类型（编译时报错）
        else {
            static_assert(!std::is_same_v<T, T>, "Unsupported type in ResultSetParser");
        }
    }

    // MYSQL_ROW类型转换分发
    template<typename T>
    std::optional<T> getValueFromRowImpl(MYSQL_ROW row, unsigned int fieldIndex, unsigned long* lengths, bool isNull, enum_field_types field_type, type_tag<T>) const {
        if (isNull) {
            return std::nullopt;
        }

        // 处理 optional 类型（非 NULL 情况）
        /*if constexpr (is_optional_type_v<T>) {
            using ValueType = typename T::value_type;
            return getValueFromRowImpl<ValueType>(row, fieldIndex, lengths, false, type_tag<ValueType>{});
        }*/
        // 处理算术类型
        if constexpr (std::is_arithmetic_v<T>) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_unsigned_v<T>) {
                    return static_cast<T>(std::stoull(row[fieldIndex]));
                }
                else {
                    return static_cast<T>(std::stoll(row[fieldIndex]));
                }
            }
            else if constexpr (std::is_floating_point_v<T>) {
                return static_cast<T>(std::stod(row[fieldIndex]));
            }
        }
        // 处理字符串类型
        else if constexpr (std::is_same_v<T, std::string>) {
            return std::string(row[fieldIndex], lengths[fieldIndex]);
        }
        // 处理时间点类型
        else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            if (field_type == MYSQL_TYPE_TIMESTAMP || field_type == MYSQL_TYPE_DATETIME)
                return utils::str_to_time_point(std::string(row[fieldIndex], lengths[fieldIndex]));
            else if (field_type == MYSQL_TYPE_DOUBLE || field_type == MYSQL_TYPE_NEWDECIMAL || field_type == MYSQL_TYPE_FLOAT || field_type == MYSQL_TYPE_DECIMAL)
                return std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(static_cast<uint64_t>(std::stod(row[fieldIndex])))
                );
            else
                throw DatabaseException("Unsupported field type for time_point conversion " __FUNCTION__ "");
        }
        // 不支持的类型（编译时报错）
        else {
            static_assert(!std::is_same_v<T, T>, "Unsupported type in ResultSetParser");
        }
    }
};

// 结果集智能指针，用于自动管理MYSQL_RES资源
struct ResultGuard {
    MYSQL_RES* res;
    ResultGuard(MYSQL_RES* r) : res(r) {}
    ~ResultGuard() { if (res) mysql_free_result(res); }
};

#pragma region UPDATE_FIELDS_BY_CONDITION_HELPER
/**
* @brief 用于 updateFieldsByCondition(<condition>, <fields>, <values>) 参数包输入限制
* @example 
* 
* "password" in MYSQL type is "VARCHAR(60)", "update_time" in MYSQL type is "TIMESTAMP(3)"
* updateFieldsByCondition(
*    condition, 
*    std::vector<std::string>{"password", "update_time"},
*    BindableParam{ varchar{"updated_password", 16} },)
*    ExpressionParam{ "NOW(3)" })
* 
* @like "UPDATE table SET password = <bindable_param>, update_time = NOW(3) WHERE <condition>"
*
* @note <varchar> see "Inl-ValueBinder.ipp" <condition> see "Inl-QueryCondition.ipp"
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

template<typename T>
struct is_bindable_param : std::false_type {};

template<typename T>
struct is_bindable_param<BindableParam<T>> : std::true_type {};

template<typename T>
struct is_expression_param : std::false_type {};

template<>
struct is_expression_param<ExpressionParam> : std::true_type {};

template<typename T>
constexpr bool is_supported_param_v =
is_bindable_param<std::decay_t<T>>::value ||
is_expression_param<std::decay_t<T>>::value;

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
#pragma endregion UPDATE_FIELDS_BY_CONDITION_HELPER
