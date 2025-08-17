#include "base_entity.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace universal::mysql {
    std::string BaseEntity::toString() const
    {
        constexpr const char* headers[] = { "Variable_name", "Value" };
        auto names = getFieldNames();
        auto values = getFieldValues();

        // 计算列宽（考虑表头和数据）
        size_t col1 = _max(names, [](const auto& v) {
            return v.size();
            }, headers[0]);
        size_t col2 = _max(values, [](const auto& v) {
            return v.size();
            }, headers[1]);

        // 辅助函数：计算最大宽度
        auto calcMaxWidth = [](const std::vector<std::string>& vec, const std::string& header) {
            size_t max = header.size();
            for (const auto& s : vec) {
                if (s.size() > max) max = s.size();
            }
            return max;
            };

        col1 = calcMaxWidth(names, headers[0]);
        col2 = calcMaxWidth(values, headers[1]);

        // 构建分隔线
        std::ostringstream oss;
        oss << "+-" << std::string(col1, '-') << "-+-" << std::string(col2, '-') << "-+\n";
        std::string separator = oss.str();
        oss.str("");

        // 构建表头
        oss << separator
            << "| " << std::left << std::setw(col1) << headers[0]
            << " | " << std::setw(col2) << headers[1] << " |\n"
            << separator;

        // 构建数据行
        for (size_t i = 0; i < names.size(); ++i) {
            oss << "| " << std::setw(col1) << names[i]
                << " | " << std::setw(col2) << values[i] << " |\n";
        }
        oss << separator;

        return oss.str();
    }

    size_t BaseEntity::_max(const std::vector<std::string>& vec, size_t(*getter)(const std::string&), const std::string& header) const
    {
        size_t max = header.size();
        for (const auto& s : vec) {
            size_t len = getter(s);
            if (len > max) max = len;
        }
        return max;
    }

}
