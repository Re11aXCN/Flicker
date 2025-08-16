#ifndef UNIVERSAL_FLAGS_H_
#define UNIVERSAL_FLAGS_H_
#include <magic_enum/magic_enum_all.hpp>
#include <type_traits>
#include <initializer_list>
#include <string>
#include <sstream>
#include <utility>
#include <cstdint>

// 类型安全标志，类似Qt的QT_TYPESAFE_FLAGS
#ifndef FLAGS_TYPESAFE
#define FLAGS_TYPESAFE 1
#endif

namespace universal::flags {

    // 标志值包装器，防止隐式整数转换
    class Flag {
        int value_;
    public:
        constexpr explicit Flag(int value) noexcept : value_(value) {}
        constexpr operator int() const noexcept { return value_; }
    };

    // 不兼容标志类，用于类型安全
    class IncompatibleFlag {
        int value_;
    public:
        constexpr explicit IncompatibleFlag(int value) noexcept : value_(value) {}
        constexpr operator int() const noexcept { return value_; }
    };

    // 零值比较辅助类
    struct CompareAgainstLiteralZero {
        constexpr CompareAgainstLiteralZero() noexcept = default;
    };

    // C++23特性：自动检测枚举范围
    template<typename Enum>
    struct enum_range_detector {
        static constexpr auto detect_range() {
            constexpr auto values = magic_enum::enum_values<Enum>();
            if constexpr (values.empty()) {
                return std::pair{ 0, 0 };
            }
            else {
                auto min_val = static_cast<int>(values.front());
                auto max_val = static_cast<int>(values.back());
                for (auto val : values) {
                    auto int_val = static_cast<int>(val);
                    min_val = int_val < min_val ? int_val : min_val;
                    max_val = max_val < int_val ? int_val : max_val;
                }
                return std::pair{ min_val, max_val };
            }
        }

        static constexpr auto range = detect_range();
        static constexpr int min = range.first;
        static constexpr int max = range.second;
    };

    // 主标志位处理模板
    template <typename Enum>
    class Flags {
        static_assert(std::is_enum_v<Enum>,
            "Flags can only be used with enumeration types");

        // 检查底层类型大小
        using Underlying = std::underlying_type_t<Enum>;
        static_assert(sizeof(Underlying) <= sizeof(int),
            "Underlying type is larger than int, potential overflow");

        // 根据底层类型选择Int类型
        using Int = std::conditional_t<
            std::is_unsigned_v<Underlying>,
            unsigned int,
            signed int
        >;

        Int value_ = 0;

    public:
        using enum_type = Enum;

        // 构造和转换
        constexpr Flags() noexcept = default;
        constexpr Flags(Enum value) noexcept : value_(static_cast<Int>(value)) {}
        constexpr Flags(Flag flag) noexcept : value_(static_cast<Int>(flag)) {}

        // 从整数创建（静态方法）
        constexpr static Flags fromInt(Int value) noexcept {
            return Flags(Flag(static_cast<int>(value)));
        }

        // 初始化列表构造
        constexpr Flags(std::initializer_list<Enum> flags) noexcept {
            for (auto f : flags) {
                value_ |= static_cast<Int>(f);
            }
        }

        constexpr bool isZero() const noexcept { return value_ == Int(0); }

#if FLAGS_TYPESAFE
        // 类型安全模式：显式转换
        constexpr explicit operator bool() const noexcept { return value_ != Int(0); }
        constexpr explicit operator Int() const noexcept { return value_; }
        constexpr explicit operator Flag() const noexcept { return Flag(static_cast<int>(value_)); }
#else
        // 非类型安全模式：隐式转换
        constexpr operator Int() const noexcept { return value_; }
        constexpr bool operator!() const noexcept { return value_ == Int(0); }
#endif

        // 赋值运算符
        constexpr Flags& operator&=(Flags other) noexcept {
            value_ &= other.value_;
            return *this;
        }

        constexpr Flags& operator|=(Flags other) noexcept {
            value_ |= other.value_;
            return *this;
        }

        constexpr Flags& operator^=(Flags other) noexcept {
            value_ ^= other.value_;
            return *this;
        }

        constexpr Flags& operator&=(Enum other) noexcept {
            value_ &= static_cast<Int>(other);
            return *this;
        }

        constexpr Flags& operator|=(Enum other) noexcept {
            value_ |= static_cast<Int>(other);
            return *this;
        }

        constexpr Flags& operator^=(Enum other) noexcept {
            value_ ^= static_cast<Int>(other);
            return *this;
        }

#if !FLAGS_TYPESAFE
        // 非类型安全模式允许整数操作
        constexpr Flags& operator&=(int mask) noexcept {
            value_ &= static_cast<Int>(mask);
            return *this;
        }
        constexpr Flags& operator&=(unsigned int mask) noexcept {
            value_ &= static_cast<Int>(mask);
            return *this;
        }
#endif

        // 位运算符
        constexpr Flags operator|(Flags other) const noexcept {
            return Flag(static_cast<int>(value_ | other.value_));
        }

        constexpr Flags operator|(Enum other) const noexcept {
            return Flag(static_cast<int>(value_ | static_cast<Int>(other)));
        }

        constexpr Flags operator&(Flags other) const noexcept {
            return Flag(static_cast<int>(value_ & other.value_));
        }

        constexpr Flags operator&(Enum other) const noexcept {
            return Flag(static_cast<int>(value_ & static_cast<Int>(other)));
        }

        constexpr Flags operator^(Flags other) const noexcept {
            return Flag(static_cast<int>(value_ ^ other.value_));
        }

        constexpr Flags operator^(Enum other) const noexcept {
            return Flag(static_cast<int>(value_ ^ static_cast<Int>(other)));
        }

        constexpr Flags operator~() const noexcept {
            return Flag(static_cast<int>(~value_));
        }

#if !FLAGS_TYPESAFE
        // 非类型安全模式允许整数位运算
        constexpr Flags operator&(int mask) const noexcept {
            return Flag(static_cast<int>(value_ & static_cast<Int>(mask)));
        }
        constexpr Flags operator&(unsigned int mask) const noexcept {
            return Flag(static_cast<int>(value_ & static_cast<Int>(mask)));
        }
#endif

        // 禁止加减运算符
        constexpr void operator+(Flags other) const noexcept = delete;
        constexpr void operator+(Enum other) const noexcept = delete;
        constexpr void operator+(int other) const noexcept = delete;
        constexpr void operator-(Flags other) const noexcept = delete;
        constexpr void operator-(Enum other) const noexcept = delete;
        constexpr void operator-(int other) const noexcept = delete;

        // 比较运算符
        constexpr friend bool operator==(Flags lhs, Flags rhs) noexcept {
            return lhs.value_ == rhs.value_;
        }

        constexpr friend bool operator!=(Flags lhs, Flags rhs) noexcept {
            return lhs.value_ != rhs.value_;
        }

        constexpr friend bool operator==(Flags lhs, Enum rhs) noexcept {
            return lhs == Flags(rhs);
        }

        constexpr friend bool operator!=(Flags lhs, Enum rhs) noexcept {
            return lhs != Flags(rhs);
        }

        constexpr friend bool operator==(Enum lhs, Flags rhs) noexcept {
            return Flags(lhs) == rhs;
        }

        constexpr friend bool operator!=(Enum lhs, Flags rhs) noexcept {
            return Flags(lhs) != rhs;
        }

#if FLAGS_TYPESAFE
        // 类型安全模式：与字面值0比较
        constexpr friend bool operator==(Flags flags, CompareAgainstLiteralZero) noexcept {
            return flags.value_ == Int(0);
        }
        constexpr friend bool operator!=(Flags flags, CompareAgainstLiteralZero) noexcept {
            return flags.value_ != Int(0);
        }
        constexpr friend bool operator==(CompareAgainstLiteralZero, Flags flags) noexcept {
            return Int(0) == flags.value_;
        }
        constexpr friend bool operator!=(CompareAgainstLiteralZero, Flags flags) noexcept {
            return Int(0) != flags.value_;
        }
#endif

        // 测试标志
        constexpr bool testFlag(Enum flag) const noexcept {
            const Int i = static_cast<Int>(flag);
            return i ? (value_ & i) == i : value_ == Int(0);
        }

        constexpr bool testFlags(Flags flags) const noexcept {
            return flags.value_ ? (value_ & flags.value_) == flags.value_ : value_ == Int(0);
        }

        constexpr bool testAnyFlag(Enum flag) const noexcept {
            return (value_ & static_cast<Int>(flag)) != Int(0);
        }

        constexpr bool testAnyFlags(Flags flags) const noexcept {
            return (value_ & flags.value_) != Int(0);
        }

        // 设置标志
        constexpr Flags& setFlag(Enum flag, bool on = true) noexcept {
            const Int i = static_cast<Int>(flag);
            value_ = on ? (value_ | i) : (value_ & ~i);
            return *this;
        }

        // 获取整数值
        constexpr Int toInt() const noexcept { return value_; }

        std::string toString() const {
            if (value_ == Int(0)) {
                if (auto name = magic_enum::enum_name(static_cast<Enum>(0)); !name.empty()) {
                    return std::string(name);
                }
                return "0";
            }

            std::string result;
            bool first = true;
            Int remaining = value_;

            // 直接获取枚举值
            constexpr auto values = magic_enum::enum_values<Enum>();

            // 第一次遍历：处理单个位标志
            for (auto value : values) {
                const Int v = static_cast<Int>(value);

                // 跳过零值
                if (v == 0) continue;

                // 检查是否是单个位标志且被设置
                if ((v & (v - 1)) == 0 && (remaining & v) == v) {
                    if (!first) result += "|";
                    if (auto name = magic_enum::enum_name(value); !name.empty()) {
                        result += name;
                    }
                    else {
                        result += std::to_string(static_cast<int>(v));
                    }
                    first = false;
                    remaining &= ~v;  // 清除已处理的位
                }
            }

            // 第二次遍历：处理剩余标志（组合值）
            if (remaining != 0) {
                for (auto value : values) {
                    const Int v = static_cast<Int>(value);

                    // 跳过零值和未设置的值
                    if (v == 0 || (remaining & v) != v) continue;

                    if (!first) result += "|";
                    if (auto name = magic_enum::enum_name(value); !name.empty()) {
                        result += name;
                    }
                    else {
                        result += std::to_string(static_cast<int>(v));
                    }
                    first = false;
                    remaining &= ~v;  // 清除已处理的位
                }
            }

            // 如果还有未处理的位，返回整数值
            if (remaining != 0) {
                return std::to_string(static_cast<int>(value_));
            }

            return result;
        }
    };

    // 全局运算符
    template <typename Enum>
    constexpr Flags<Enum> operator|(Enum lhs, Enum rhs) noexcept {
        return Flags<Enum>(lhs) | rhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator&(Enum lhs, Enum rhs) noexcept {
        return Flags<Enum>(lhs) & rhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator^(Enum lhs, Enum rhs) noexcept {
        return Flags<Enum>(lhs) ^ rhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator|(Enum lhs, Flags<Enum> rhs) noexcept {
        return rhs | lhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator&(Enum lhs, Flags<Enum> rhs) noexcept {
        return rhs & lhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator^(Enum lhs, Flags<Enum> rhs) noexcept {
        return rhs ^ lhs;
    }

    template <typename Enum>
    constexpr Flags<Enum> operator~(Enum value) noexcept {
        return ~Flags<Enum>(value);
    }

    // 声明宏
#define DECLARE_FLAGS(FlagsName, EnumType) \
    using FlagsName = universal::flags::Flags<EnumType>;

// 类型安全运算符声明宏
#if FLAGS_TYPESAFE
#define DECLARE_TYPESAFE_OPERATORS_FOR_FLAGS_ENUM(_Flags) \
    [[maybe_unused]] \
    constexpr inline _Flags operator~(_Flags::enum_type e) noexcept \
    { return ~_Flags(e); } \
    [[maybe_unused]] \
    constexpr inline void operator|(_Flags::enum_type f1, int f2) noexcept = delete;
#else
#define DECLARE_TYPESAFE_OPERATORS_FOR_FLAGS_ENUM(_Flags) \
    [[maybe_unused]] \
    constexpr inline universal::flags::IncompatibleFlag operator|(_Flags::enum_type f1, int f2) noexcept \
    { return universal::flags::IncompatibleFlag(int(f1) | f2); }
#endif

#define DECLARE_OPERATORS_FOR_FLAGS(_Flags) \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator|(_Flags::enum_type f1, _Flags::enum_type f2) noexcept \
    { return universal::flags::Flags<_Flags::enum_type>(f1) | f2; } \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator|(_Flags::enum_type f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept \
    { return f2 | f1; } \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator&(_Flags::enum_type f1, _Flags::enum_type f2) noexcept \
    { return universal::flags::Flags<_Flags::enum_type>(f1) & f2; } \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator&(_Flags::enum_type f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept \
    { return f2 & f1; } \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator^(_Flags::enum_type f1, _Flags::enum_type f2) noexcept \
    { return universal::flags::Flags<_Flags::enum_type>(f1) ^ f2; } \
    [[maybe_unused]] \
    constexpr inline universal::flags::Flags<_Flags::enum_type> operator^(_Flags::enum_type f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept \
    { return f2 ^ f1; } \
    constexpr inline void operator+(_Flags::enum_type f1, _Flags::enum_type f2) noexcept = delete; \
    constexpr inline void operator+(_Flags::enum_type f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept = delete; \
    constexpr inline void operator+(int f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept = delete; \
    constexpr inline void operator-(_Flags::enum_type f1, _Flags::enum_type f2) noexcept = delete; \
    constexpr inline void operator-(_Flags::enum_type f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept = delete; \
    constexpr inline void operator-(int f1, universal::flags::Flags<_Flags::enum_type> f2) noexcept = delete; \
    constexpr inline void operator+(int f1, _Flags::enum_type f2) noexcept = delete; \
    constexpr inline void operator+(_Flags::enum_type f1, int f2) noexcept = delete; \
    constexpr inline void operator-(int f1, _Flags::enum_type f2) noexcept = delete; \
    constexpr inline void operator-(_Flags::enum_type f1, int f2) noexcept = delete; \
    DECLARE_TYPESAFE_OPERATORS_FOR_FLAGS_ENUM(_Flags)

// 支持与字面值0比较的宏
#if FLAGS_TYPESAFE
    // 在类型安全模式下，允许与字面值0比较
    constexpr inline CompareAgainstLiteralZero Zero = CompareAgainstLiteralZero{};
#endif

} // namespace universal::flags

#endif // !UNIVERSAL_FLAGS_H_
