#ifndef UNIVERSAL_MYSQL_RESULT_SET_PARSER_H_
#define UNIVERSAL_MYSQL_RESULT_SET_PARSER_H_
#include <optional>
#include <chrono>
#include <string>
#include <type_traits>
#include <mysql.h>
#include "cxx_utils.h"
namespace universal::mysql {
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
                    return universal::mysql::time::mysqlTimeToChrono(*static_cast<MYSQL_TIME*>(bind->buffer));
                else if (field_type == MYSQL_TYPE_DOUBLE || field_type == MYSQL_TYPE_NEWDECIMAL || field_type == MYSQL_TYPE_FLOAT || field_type == MYSQL_TYPE_DECIMAL)
                    return std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(static_cast<uint64_t>(*static_cast<double*>(bind->buffer)))
                    );
                else
                    throw std::runtime_error("Unsupported field type for time_point conversion " __FUNCTION__ "");
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
                    return universal::utils::time::str_to_time_point(std::string(row[fieldIndex], lengths[fieldIndex]));
                else if (field_type == MYSQL_TYPE_DOUBLE || field_type == MYSQL_TYPE_NEWDECIMAL || field_type == MYSQL_TYPE_FLOAT || field_type == MYSQL_TYPE_DECIMAL)
                    return std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(static_cast<uint64_t>(std::stod(row[fieldIndex])))
                    );
                else
                    throw std::runtime_error("Unsupported field type for time_point conversion " __FUNCTION__ "");
            }
            // 不支持的类型（编译时报错）
            else {
                static_assert(!std::is_same_v<T, T>, "Unsupported type in ResultSetParser");
            }
        }
    };

}

#endif // !UNIVERSAL_MYSQL_RESULT_SET_PARSER_H_