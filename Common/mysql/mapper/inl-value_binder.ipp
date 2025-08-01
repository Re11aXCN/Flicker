struct varchar { // stmt bind string type
    const char* val;
    unsigned long len;
    varchar(const char* v, unsigned long l) : val(v), len(l) {}
    ~varchar() {}
    varchar(const varchar&) = default;
    varchar& operator=(const varchar&) = default;
    varchar(varchar&&) = default;
    varchar& operator=(varchar&&) = default;
};
template<typename Type>
struct is_varchar_type : std::false_type {};
template<>
struct is_varchar_type<varchar> : std::true_type {};
template<typename Type>
inline constexpr bool is_varchar_type_v = is_varchar_type<Type>::value;

class ValueBinder {
public:
    // 使用折叠表达式替代递归
    template<typename... Args>
    static void bindParams(MYSQL_BIND* binds, Args&&... args) {
        size_t index = 0;
        (bindSingleValue(binds[index++], std::forward<Args>(args)), ...);
    }

    // 单个值绑定
    template<typename ValueType>
    static void bindSingleValue(MYSQL_BIND& bind, ValueType&& value)
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
        else if constexpr (is_varchar_type_v<RawType>) {
            bind.buffer_type = MYSQL_TYPE_VAR_STRING;
            bind.buffer = const_cast<char*>(value.val);
            bind.buffer_length = value.len + 1;
            bind.length = const_cast<unsigned long*>(&value.len);// &value.len;
            bind.is_null = 0;
        }
        /*else if constexpr (std::is_same_v<RawType, MYSQL_TIME>) {
            bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
            bind.buffer = const_cast<MYSQL_TIME*>(&value);
            bind.buffer_length = sizeof(value);
            bind.is_null = 0;
            bind.length = 0;
        }*/
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
            throw DatabaseException("Unsupported value type in bindSingleValue");
        }
    }
};