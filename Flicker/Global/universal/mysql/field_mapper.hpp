#ifndef UNIVERSAL_FIELD_MAPPER_HPP_
#define UNIVERSAL_FIELD_MAPPER_HPP_

#include <array>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <type_traits>
#include <tuple>
namespace universal::mysql {

// 辅助类型定义
template <typename... Ts>
using VariantType = std::variant<Ts...>;

// 类型列表操作
template <typename... Ts>
struct TypeList {
    static constexpr size_t size = sizeof...(Ts);
};

// 合并类型列表
template <typename List1, typename List2>
struct Concat;

template <typename... Ts1, typename... Ts2>
struct Concat<TypeList<Ts1...>, TypeList<Ts2...>> {
    using type = TypeList<Ts1..., Ts2...>;
};

// 提取唯一类型集合
template <typename List>
struct UniqueTypes;

template <>
struct UniqueTypes<TypeList<>> {
    using type = TypeList<>;
};

template <typename T, typename... Ts>
struct UniqueTypes<TypeList<T, Ts...>> {
    using Rest = typename UniqueTypes<TypeList<Ts...>>::type;
    using type = std::conditional_t<
        (std::is_same_v<T, Ts> || ...),
        Rest,
        typename Concat<TypeList<T>, Rest>::type
    >;
};

// 获取类型列表第I个元素
template <size_t I, typename List>
struct TypeAt;

template <size_t I, typename Head, typename... Tail>
struct TypeAt<I, TypeList<Head, Tail...>> : TypeAt<I - 1, TypeList<Tail...>> {};

template <typename Head, typename... Tail>
struct TypeAt<0, TypeList<Head, Tail...>> {
    using type = Head;
};

template <size_t I, typename List>
using TypeAt_t = typename TypeAt<I, List>::type;

// 类型列表转Variant
template <typename List>
struct TypeListToVariant;

template <typename... Ts>
struct TypeListToVariant<TypeList<Ts...>> {
    using type = VariantType<Ts...>;
};

// =================================================================
// 模式1：显式字段映射器（需要显式指定字段类型和成员指针）
// =================================================================
template <typename T, typename... Fields>
class ExplicitFieldMapper {
public:
    using FieldTypes = TypeList<std::decay_t<Fields>...>;
    using VariantType = typename TypeListToVariant<
        typename UniqueTypes<typename FieldTypes>::type
    >::type;
    using VariantMap = std::unordered_map<std::string_view, VariantType>;

    ExplicitFieldMapper(Fields T::*... fields) : fields_{ fields... } {}
    virtual ~ExplicitFieldMapper() = default;

    // 填充映射表
    void populateMap(typename VariantMap& map) const {
        _populateImpl(map, std::index_sequence_for<Fields...>{});
    }
private:
    template <size_t... Is>
    void _populateImpl(typename VariantMap& map,
        std::index_sequence<Is...>) const {
        (_populateSingle<Is>(map), ...);
    }

    template <size_t I>
    void _populateSingle(typename VariantMap& map) const {
        using MemberPtrType = std::tuple_element_t<I, std::tuple<Fields T::*...>>;
        using MemberType = typename std::decay_t<decltype(std::declval<T>().*std::declval<MemberPtrType>())>;

        // 创建类型默认值作为占位符
        map[fieldNames_[I]] = MemberType{};
    }

    inline static const std::array<const char*, sizeof...(Fields)> fieldNames_ = T::FIELD_NAMES;

    std::tuple<Fields T::*...> fields_;
};

// =================================================================
// 模式2：自省字段映射器（从类定义中提取字段信息）
// =================================================================
template <typename T>
class IntrospectiveFieldMapper {
    // 静态断言确保用户类定义了必要的元信息
    static_assert(requires { typename T::FieldTypeList; },
        "T must define FieldTypeList as TypeList<...>");
    static_assert(requires { T::FIELD_NAMES; },
        "T must define FIELD_NAMES as constexpr std::array");

    // 从用户类提取字段数量
    static constexpr size_t FieldCount = std::tuple_size_v<decltype(T::FIELD_NAMES)>;
public:
    // 提取字段类型列表
    using FieldTypes = typename T::FieldTypeList;
    using VariantType = typename TypeListToVariant<
        typename UniqueTypes<typename FieldTypes>::type
    >::type;
    using VariantMap = std::unordered_map<std::string_view, VariantType>;

    // 构造函数：编译时验证成员指针类型
    template <typename... MemberPointers>
    constexpr IntrospectiveFieldMapper(MemberPointers... pointers) {
        /*static_assert(sizeof...(MemberPointers) == FieldCount,
            "Number of member pointers must match field count");*/
        static_assert(FieldTypes::size == FieldCount,
            "Field type count must match name count");
        // 编译时验证每个成员指针类型
        _validateMemberTypes<0>(pointers...);
    }

    virtual ~IntrospectiveFieldMapper() = default;

    void populateMap(VariantMap& map) const {
        _populateImpl(map, std::make_index_sequence<FieldCount>{});
    }

private:
    template <size_t I, typename Ptr, typename... Rest>
    constexpr void _validateMemberTypes(Ptr ptr, Rest... rest) {
        using ExpectedType = TypeAt_t<I, FieldTypes>;
        using ActualType = std::remove_cvref_t<
            decltype(std::declval<T>().*std::declval<Ptr>())>;

        static_assert(std::is_same_v<ExpectedType, ActualType>,
            "Member pointer type does not match declared field type");

        if constexpr (sizeof...(Rest) > 0) {
            _validateMemberTypes<I + 1>(rest...);
        }
    }
    // 终止递归的base case
    template <size_t I>
    constexpr void _validateMemberTypes() {}

    template <size_t... Is>
    void _populateImpl(VariantMap& map, std::index_sequence<Is...>) const {
        (_populateSingle<Is>(map), ...);
    }

    template <size_t I>
    void _populateSingle(VariantMap& map) const {
        using MemberType = TypeAt_t<I, FieldTypes>;
        map[T::FIELD_NAMES[I]] = MemberType{};
    }
};

}
#endif // !FIELD_MAPPER_HPP_