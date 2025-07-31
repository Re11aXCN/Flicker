// 一个能在编译期构造的、固定长度的字符串类
template<size_t N>
struct fixed_string {
    std::array<char, N> data{};

    consteval fixed_string(const char(&str)[N]) {
        std::copy_n(str, N, data.begin());
    }

    auto operator<=>(const fixed_string&) const = default;

    constexpr size_t size() const { return N - 1; }
    constexpr const char* c_str() const { return data.data(); }
    constexpr operator std::string_view() const { return { data.data(), size() }; }
};

template<size_t N>
fixed_string(const char(&)[N]) -> fixed_string<N>;

enum class SortOrder {
    Ascending,  // 升序
    Descending  // 降序
};
template<size_t N>
struct OrderBy {
    SortOrder order;
    fixed_string<N + 1> fieldName; // +1 是为了包含'\0' 结尾符

    auto operator<=>(const OrderBy&) const = default;
};

struct Pagination {
    size_t limit;
    size_t offset;
};