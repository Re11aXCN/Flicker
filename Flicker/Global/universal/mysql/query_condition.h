#ifndef UNIVERSAL_MYSQL_QUERY_CONDITION_H_
#define UNIVERSAL_MYSQL_QUERY_CONDITION_H_
#include <string>
#include <vector>
#include "cxx_utils.h"
#include "stmt_ptr.h"
namespace universal::mysql {
enum class ConditionType {
    TRUE_,
    FALSE_,

    EQ_,
    NEQ_,
    GT_E_,
    LT_E_,
    BETWEEN_,

    LIKE_,
    REGEXP_,

    IN_,
    NOTIN_,

    IS_N_NULL_,

    AND_,
    OR_,
    NOT_,

    RAW_
};
// 查询条件基类
class IQueryCondition {
public:
    virtual ~IQueryCondition() = default;

    // 生成SQL条件子句
    virtual std::string buildConditionClause() const = 0;

    // 绑定参数到预处理语句
    virtual void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const = 0;

    // 获取参数数量
    virtual size_t getParamCount() const = 0;

    virtual ConditionType getConditionType() const = 0;
};

class TrueCondition : public IQueryCondition {
public:
    TrueCondition() {}
    virtual ~TrueCondition() = default;

    std::string buildConditionClause() const override {
        return "1=1";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {

    }

    size_t getParamCount() const override {
        return 0;
    }

    ConditionType getConditionType() const override {
        return ConditionType::TRUE_;
    }
};

class FalseCondition : public IQueryCondition {
public:
    FalseCondition() {}
    virtual ~FalseCondition() = default;

    std::string buildConditionClause() const override {
        return "1=0";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {

    }

    size_t getParamCount() const override {
        return 0;
    }

    ConditionType getConditionType() const override {
        return ConditionType::FALSE_;
    }
};

// 值条件类
#pragma region VALUE_CONDITIONS 
// 等值条件
template<typename T>
class EqualCondition : public IQueryCondition {
private:
    std::string _fieldName;
    T _value;

public:
    EqualCondition(const std::string& fieldName, const T& value)
        : _fieldName(fieldName), _value(value) {
    }
    EqualCondition(std::string&& fieldName, T&& value)
        : _fieldName(std::move(fieldName)), _value(std::move(value)) {
    }

    virtual ~EqualCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + " = ?";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 使用ValueBinder绑定参数
        bindSingleValue(binds[bindIndex++], _value);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const T& getValue() const {
        return _value;
    }

    ConditionType getConditionType() const override {
        return ConditionType::EQ_;
    }
};

// 不等值条件
template<typename T>
class NotEqualCondition : public IQueryCondition {
private:
    std::string _fieldName;
    T _value;

public:
    NotEqualCondition(const std::string& fieldName, const T& value)
        : _fieldName(fieldName), _value(value) {
    }
    NotEqualCondition(std::string&& fieldName, T&& value)
        : _fieldName(std::move(fieldName)), _value(std::move(value)) {
    }

    virtual ~NotEqualCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + " <> ?";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 使用ValueBinder绑定参数
        bindSingleValue(binds[bindIndex++], _value);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const T& getValue() const {
        return _value;
    }

    ConditionType getConditionType() const override {
        return ConditionType::NEQ_;
    }
};

// 大于条件
template<typename T>
class GreaterThanCondition : public IQueryCondition {
private:
    std::string _fieldName;
    T _value;
    bool _orEqual;

public:
    GreaterThanCondition(const std::string& fieldName, const T& value, bool orEqual = false)
        : _fieldName(fieldName), _value(value), _orEqual(orEqual) {
    }
    GreaterThanCondition(std::string&& fieldName, T&& value, bool orEqual = false)
        : _fieldName(std::move(fieldName)), _value(std::move(value)), _orEqual(orEqual) {
    }

    virtual ~GreaterThanCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + (_orEqual ? " >= ?" : " > ?");
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定值
        bindSingleValue(binds[bindIndex++], _value);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const T& getValue() const {
        return _value;
    }

    ConditionType getConditionType() const override {
        return ConditionType::GT_E_;
    }
};

// 小于条件
template<typename T>
class LessThanCondition : public IQueryCondition {
private:
    std::string _fieldName;
    T _value;
    bool _orEqual;

public:
    LessThanCondition(const std::string& fieldName, const T& value, bool orEqual = false)
        : _fieldName(fieldName), _value(value), _orEqual(orEqual) {
    }
    LessThanCondition(std::string&& fieldName, T&& value, bool orEqual = false)
        : _fieldName(std::move(fieldName)), _value(std::move(value)), _orEqual(orEqual) {
    }

    virtual ~LessThanCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + (_orEqual ? " <= ?" : " < ?");
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定值
        bindSingleValue(binds[bindIndex++], _value);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const T& getValue() const {
        return _value;
    }

    ConditionType getConditionType() const override {
        return ConditionType::LT_E_;
    }
};
#pragma endregion VALUE_CONDITIONS

#pragma region RANGE_CONDITIONS
// 范围条件 (BETWEEN)
template<typename T>
class BetweenCondition : public IQueryCondition {
private:
    std::string _fieldName;
    T _minValue;
    T _maxValue;

public:
    BetweenCondition(const std::string& fieldName, const T& minValue, const T& maxValue)
        : _fieldName(fieldName), _minValue(minValue), _maxValue(maxValue) {
    }
    BetweenCondition(std::string&& fieldName, T&& minValue, T&& maxValue)
        : _fieldName(std::move(fieldName)), _minValue(std::move(minValue)), _maxValue(std::move(maxValue)) {
    }

    virtual ~BetweenCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + " BETWEEN ? AND ?";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定最小值和最大值
        bindSingleValue(binds[bindIndex++], _minValue);
        bindSingleValue(binds[bindIndex++], _maxValue);
    }

    size_t getParamCount() const override {
        return 2;
    }

    const T& getMinValue() const {
        return _minValue;
    }

    const T& getMaxValue() const {
        return _maxValue;
    }

    ConditionType getConditionType() const override {
        return ConditionType::BETWEEN_;
    }
};
#pragma endregion RANGE_CONDITIONS

#pragma region FUZZY_MATCHING
// 模糊匹配条件 (LIKE)
class LikeCondition : public IQueryCondition {
private:
    std::string _fieldName;
    mysql_varchar _pattern;

public:
    LikeCondition(const std::string& fieldName, const mysql_varchar& pattern)
        : _fieldName(fieldName), _pattern(pattern) {
    }
    LikeCondition(std::string&& fieldName, mysql_varchar&& pattern)
        : _fieldName(std::move(fieldName)), _pattern(std::move(pattern)) {
    }

    virtual ~LikeCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + " LIKE ?";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定模式字符串
        bindSingleValue(binds[bindIndex++], _pattern);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const std::string_view& getPattern() const {
        return std::string_view(_pattern.val, _pattern.len);
    }

    ConditionType getConditionType() const override {
        return ConditionType::LIKE_;
    }
};

// 正则表达式条件 (REGEXP)
class RegexpCondition : public IQueryCondition {
private:
    std::string _fieldName;
    mysql_varchar _pattern;

public:
    RegexpCondition(const std::string& fieldName, const mysql_varchar& pattern)
        : _fieldName(fieldName), _pattern(pattern) {
    }
    RegexpCondition(std::string&& fieldName, mysql_varchar&& pattern)
        : _fieldName(std::move(fieldName)), _pattern(std::move(pattern)) {
    }

    virtual ~RegexpCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + " REGEXP ?";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定正则表达式模式
        bindSingleValue(binds[bindIndex++], _pattern);
    }

    size_t getParamCount() const override {
        return 1;
    }

    const std::string_view& getPattern() const {
        return std::string_view(_pattern.val, _pattern.len);
    }

    ConditionType getConditionType() const override {
        return ConditionType::REGEXP_;
    }
};
#pragma endregion FUZZY_MATCHING

#pragma region SET_CONDITIONS
// IN条件
template<typename T>
class InCondition : public IQueryCondition {
private:
    std::string _fieldName;
    std::vector<T> _values;

public:
    InCondition(const std::string& fieldName, const std::vector<T>& values)
        : _fieldName(fieldName), _values(values) {
    }
    InCondition(std::string&& fieldName, std::vector<T>&& values)
        : _fieldName(std::move(fieldName)), _values(std::move(values)) {
    }

    virtual ~InCondition() = default;

    std::string buildConditionClause() const override {
        if (_values.empty()) {
            return "1=0"; // 空IN列表，永远为false
        }

        std::string placeholders;
        for (size_t i = 0; i < _values.size(); ++i) {
            if (i > 0) placeholders += ", ";
            placeholders += "?";
        }

        return _fieldName + " IN (" + placeholders + ")";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定所有IN条件的值
        for (const auto& value : _values) {
            bindSingleValue(binds[bindIndex++], value);
        }
    }

    size_t getParamCount() const override {
        return _values.size();
    }

    const std::vector<T>& getValues() const {
        return _values;
    }

    ConditionType getConditionType() const override {
        return ConditionType::IN_;
    }
};

// NOT IN条件
template<typename T>
class NotInCondition : public IQueryCondition {
private:
    std::string _fieldName;
    std::vector<T> _values;

public:
    NotInCondition(const std::string& fieldName, const std::vector<T>& values)
        : _fieldName(fieldName), _values(values) {
    }
    NotInCondition(std::string&& fieldName, std::vector<T>&& values)
        : _fieldName(std::move(fieldName)), _values(std::move(values)) {
    }

    virtual ~NotInCondition() = default;

    std::string buildConditionClause() const override {
        if (_values.empty()) {
            return "1=0"; // 空NOT IN列表，永远为false
        }

        std::string placeholders;
        for (size_t i = 0; i < _values.size(); ++i) {
            if (i > 0) placeholders += ", ";
            placeholders += "?";
        }

        return _fieldName + " NOT IN (" + placeholders + ")";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 绑定所有NOT IN条件的值
        for (const auto& value : _values) {
            bindSingleValue(binds[bindIndex++], value);
        }
    }

    size_t getParamCount() const override {
        return _values.size();
    }

    const std::vector<T>& getValues() const {
        return _values;
    }

    ConditionType getConditionType() const override {
        return ConditionType::NOTIN_;
    }
};
#pragma endregion SET_CONDITIONS

#pragma region NULL_CHECK
// NULL检查条件
class IsNullCondition : public IQueryCondition {
private:
    std::string _fieldName;
    bool _isNotNull;

public:
    IsNullCondition(const std::string& fieldName, bool isNotNull = false)
        : _fieldName(fieldName), _isNotNull(isNotNull) {
    }
    IsNullCondition(std::string&& fieldName, bool isNotNull = false)
        : _fieldName(std::move(fieldName)), _isNotNull(isNotNull) {
    }

    virtual ~IsNullCondition() = default;

    std::string buildConditionClause() const override {
        return _fieldName + (_isNotNull ? " IS NOT NULL" : " IS NULL");
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // IS NULL 条件不需要绑定参数
    }

    size_t getParamCount() const override {
        return 0;
    }

    ConditionType getConditionType() const override {
        return ConditionType::IS_N_NULL_;
    }
};
#pragma endregion NULL_CHECK

#pragma region COMPOSITE_CONDITIONS
// 复合条件 - AND
class AndCondition : public IQueryCondition {
private:
    std::vector<std::unique_ptr<IQueryCondition>> _conditions;

public:
    AndCondition() {}

    // vector会拷贝元素，但是unique_ptr不允许拷贝，这样定义是错误的
    /*AndCondition(const std::vector<std::unique_ptr<IQueryCondition>>& conditions)
        : _conditions(conditions) {
    }*/
    AndCondition(const std::vector<std::unique_ptr<IQueryCondition>>&) = delete;
    AndCondition(std::vector<std::unique_ptr<IQueryCondition>>&) = delete;

    AndCondition(std::vector<std::unique_ptr<IQueryCondition>>&& conditions)
        : _conditions(std::move(conditions)) {
    }

    virtual ~AndCondition() = default;

    void addCondition(std::unique_ptr<IQueryCondition> condition) {
        _conditions.push_back(std::move(condition));
    }

    std::string buildConditionClause() const override {
        if (_conditions.empty()) {
            return "1=0"; // 无条件，永远为false
        }

        std::string clause;
        for (size_t i = 0; i < _conditions.size(); ++i) {
            if (i > 0) clause += " AND ";
            clause += "(" + _conditions[i]->buildConditionClause() + ")";
        }

        return clause;
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        for (const auto& condition : _conditions) {
            condition->bind(stmtPtr, binds, bindIndex);
        }
    }

    size_t getParamCount() const override {
        size_t count = 0;
        for (const auto& condition : _conditions) {
            count += condition->getParamCount();
        }
        return count;
    }

    const std::vector<std::unique_ptr<IQueryCondition>>& getConditions() const {
        return _conditions;
    }

    ConditionType getConditionType() const override {
        return ConditionType::AND_;
    }
};

// 复合条件 - OR
class OrCondition : public IQueryCondition {
private:
    std::vector<std::unique_ptr<IQueryCondition>> _conditions;

public:
    OrCondition() {}

    OrCondition(const std::vector<std::unique_ptr<IQueryCondition>>&) = delete;
    OrCondition(std::vector<std::unique_ptr<IQueryCondition>>&) = delete;

    OrCondition(std::vector<std::unique_ptr<IQueryCondition>>&& conditions)
        : _conditions(std::move(conditions)) {
    }

    virtual ~OrCondition() = default;

    void addCondition(std::unique_ptr<IQueryCondition> condition) {
        _conditions.push_back(std::move(condition));
    }

    std::string buildConditionClause() const override {
        if (_conditions.empty()) {
            return "1=0"; // 无条件，永远为false
        }

        std::string clause;
        for (size_t i = 0; i < _conditions.size(); ++i) {
            if (i > 0) clause += " OR ";
            clause += "(" + _conditions[i]->buildConditionClause() + ")";
        }

        return clause;
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        for (const auto& condition : _conditions) {
            condition->bind(stmtPtr, binds, bindIndex);
        }
    }

    size_t getParamCount() const override {
        size_t count = 0;
        for (const auto& condition : _conditions) {
            count += condition->getParamCount();
        }
        return count;
    }

    const std::vector<std::unique_ptr<IQueryCondition>>& getConditions() const {
        return _conditions;
    }

    ConditionType getConditionType() const override {
        return ConditionType::OR_;
    }
};

// NOT条件
class NotCondition : public IQueryCondition {
private:
    std::unique_ptr<IQueryCondition> _condition;

public:
    NotCondition(std::unique_ptr<IQueryCondition> condition)
        : _condition(std::move(condition)) {
    }

    virtual ~NotCondition() = default;

    std::string buildConditionClause() const override {
        return "NOT (" + _condition->buildConditionClause() + ")";
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // 将绑定委托给内部条件
        _condition->bind(stmtPtr, binds, bindIndex);
    }

    size_t getParamCount() const override {
        return _condition->getParamCount();
    }

    const std::unique_ptr<IQueryCondition>& getCondition() const {
        return _condition;
    }

    ConditionType getConditionType() const override {
        return ConditionType::NOT_;
    }
};
#pragma endregion COMPOSITE_CONDITIONS

// 原始SQL条件（直接使用SQL片段）
class RawCondition : public IQueryCondition {
private:
    std::string _sqlFragment;
    size_t _paramCount;

public:
    RawCondition(const std::string& sqlFragment, size_t paramCount = 0)
        : _sqlFragment(sqlFragment), _paramCount(paramCount) {
    }

    RawCondition(std::string&& sqlFragment, size_t paramCount = 0)
        : _sqlFragment(std::move(sqlFragment)), _paramCount(paramCount) {
    }

    virtual ~RawCondition() = default;

    std::string buildConditionClause() const override {
        return _sqlFragment;
    }

    void bind(StmtPtr& stmtPtr, MYSQL_BIND* binds, size_t& bindIndex) const override {
        // RawCondition不处理参数绑定，由调用者负责
    }

    size_t getParamCount() const override {
        return _paramCount;
    }

    ConditionType getConditionType() const override {
        return ConditionType::RAW_;
    }
};

// 条件构建器 - 用于方便地创建条件
class QueryConditionBuilder {
public:
    static std::unique_ptr<IQueryCondition> true_() {
        return std::make_unique<TrueCondition>();
    }
    static std::unique_ptr<IQueryCondition> false_() {
        return std::make_unique<FalseCondition>();
    }

    // 等值条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> eq_(const std::string& fieldName, const T& value) {
        return std::make_unique<EqualCondition<T>>(fieldName, value);
    }

    // 不等值条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> neq_(const std::string& fieldName, const T& value) {
        return std::make_unique<NotEqualCondition<T>>(fieldName, value);
    }

    // 大于条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> gt_(const std::string& fieldName, const T& value) {
        return std::make_unique<GreaterThanCondition<T>>(fieldName, value, false);
    }

    // 大于等于条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> ge_(const std::string& fieldName, const T& value) {
        return std::make_unique<GreaterThanCondition<T>>(fieldName, value, true);
    }

    // 小于条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> lt_(const std::string& fieldName, const T& value) {
        return std::make_unique<LessThanCondition<T>>(fieldName, value, false);
    }

    // 小于等于条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> le_(const std::string& fieldName, const T& value) {
        return std::make_unique<LessThanCondition<T>>(fieldName, value, true);
    }

    // 范围条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> between_(const std::string& fieldName, const T& minValue, const T& maxValue) {
        return std::make_unique<BetweenCondition<T>>(fieldName, minValue, maxValue);
    }

    // 模糊匹配条件
    static std::unique_ptr<IQueryCondition> like_(const std::string& fieldName, const mysql_varchar& pattern) {
        return std::make_unique<LikeCondition>(fieldName, pattern);
    }

    // 正则表达式条件
    static std::unique_ptr<IQueryCondition> regexp_(const std::string& fieldName, const mysql_varchar& pattern) {
        return std::make_unique<RegexpCondition>(fieldName, pattern);
    }

    // IN条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> in_(const std::string& fieldName, const std::vector<T>& values) {
        return std::make_unique<InCondition<T>>(fieldName, values);
    }

    // NOT IN条件
    template<typename T>
    static std::unique_ptr<IQueryCondition> notIn_(const std::string& fieldName, const std::vector<T>& values) {
        return std::make_unique<NotInCondition<T>>(fieldName, values);
    }

    // IS NULL条件
    static std::unique_ptr<IQueryCondition> isNull_(const std::string& fieldName) {
        return std::make_unique<IsNullCondition>(fieldName, false);
    }

    // IS NOT NULL条件
    static std::unique_ptr<IQueryCondition> isNotNull_(const std::string& fieldName) {
        return std::make_unique<IsNullCondition>(fieldName, true);
    }

    // AND条件
    static std::unique_ptr<AndCondition> and_() {
        return std::make_unique<AndCondition>();
    }

    // OR条件
    static std::unique_ptr<OrCondition> or_() {
        return std::make_unique<OrCondition>();
    }

    // NOT条件
    static std::unique_ptr<IQueryCondition> not_(std::unique_ptr<IQueryCondition> condition) {
        return std::make_unique<NotCondition>(std::move(condition));
    }

    // 原始SQL条件
    static std::unique_ptr<IQueryCondition> raw_(const std::string& sqlFragment, size_t paramCount = 0) {
        return std::make_unique<RawCondition>(sqlFragment, paramCount);
    }

    //***** move *****//
    template<typename T>
    static std::unique_ptr<IQueryCondition> eq_(std::string&& fieldName, T&& value) {
        return std::make_unique<EqualCondition<T>>(std::move(fieldName), std::move(value));
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> neq_(std::string&& fieldName, T&& value) {
        return std::make_unique<NotEqualCondition<T>>(std::move(fieldName), std::move(value));
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> gt_(std::string&& fieldName, T&& value) {
        return std::make_unique<GreaterThanCondition<T>>(std::move(fieldName), std::move(value), false);
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> ge_(std::string&& fieldName, T&& value) {
        return std::make_unique<GreaterThanCondition<T>>(std::move(fieldName), std::move(value), true);
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> lt_(std::string&& fieldName, T&& value) {
        return std::make_unique<LessThanCondition<T>>(std::move(fieldName), std::move(value), false);
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> le_(std::string&& fieldName, T&& value) {
        return std::make_unique<LessThanCondition<T>>(std::move(fieldName), std::move(value), true);
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> between_(std::string&& fieldName, T&& minValue, T&& maxValue) {
        return std::make_unique<BetweenCondition<T>>(std::move(fieldName), std::move(minValue), std::move(maxValue));
    }

    static std::unique_ptr<IQueryCondition> like_(std::string&& fieldName, mysql_varchar&& pattern) {
        return std::make_unique<LikeCondition>(std::move(fieldName), std::move(pattern));
    }

    static std::unique_ptr<IQueryCondition> regexp_(std::string&& fieldName, mysql_varchar&& pattern) {
        return std::make_unique<RegexpCondition>(std::move(fieldName), std::move(pattern));
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> in_(std::string&& fieldName, std::vector<T>&& values) {
        return std::make_unique<InCondition<T>>(std::move(fieldName), std::move(values));
    }

    template<typename T>
    static std::unique_ptr<IQueryCondition> notIn_(std::string&& fieldName, std::vector<T>&& values) {
        return std::make_unique<NotInCondition<T>>(std::move(fieldName), std::move(values));
    }

    static std::unique_ptr<IQueryCondition> isNull_(std::string&& fieldName) {
        return std::make_unique<IsNullCondition>(std::move(fieldName), false);
    }

    static std::unique_ptr<IQueryCondition> isNotNull_(std::string&& fieldName) {
        return std::make_unique<IsNullCondition>(std::move(fieldName), true);
    }

    static std::unique_ptr<AndCondition> and_(std::vector<std::unique_ptr<IQueryCondition>>&& conditions) {
        return std::make_unique<AndCondition>(std::move(conditions));
    }

    static std::unique_ptr<OrCondition> or_(std::vector<std::unique_ptr<IQueryCondition>>&& conditions) {
        return std::make_unique<OrCondition>(std::move(conditions));
    }

    static std::unique_ptr<IQueryCondition> raw_(std::string&& sqlFragment, size_t paramCount = 0) {
        return std::make_unique<RawCondition>(std::move(sqlFragment), paramCount);
    }
};
} // namespace universal::mysql
#endif // !UNIVERSAL_MYSQL_QUERY_CONDITION_H_