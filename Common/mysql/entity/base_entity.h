#ifndef BASE_ENTITY_H_
#define BASE_ENTITY_H_

#include <string>
#include <vector>

// 基类：定义表格打印的通用算法
class BaseEntity {
public:
    virtual ~BaseEntity() = default;

    // 纯虚函数：子类必须实现字段名和字段值的获取
    virtual std::vector<std::string> getFieldNames() const = 0;
    virtual std::vector<std::string> getFieldValues() const = 0;

    // 模板方法：实现通用表格格式化
    std::string toString() const;

private:
    // 辅助函数：计算带header的最大宽度
    size_t _max(const std::vector<std::string>& vec,
        size_t(*getter)(const std::string&),
        const std::string& header) const;
};

#endif // !BASE_ENTITY_H_