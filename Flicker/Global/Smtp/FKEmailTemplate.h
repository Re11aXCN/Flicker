#ifndef FK_EMAIL_TEMPLATE_H_
#define FK_EMAIL_TEMPLATE_H_

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>  // 添加sort所需头文件
#include "universal/file_read.h"

class FKEmailTemplate {
public:
    enum class TemplateError {
        InvalidValueCount,
        PlacehlderNoInit,
    };

    // 使用结构体存储占位符及其所有位置
    struct ReplacementInfo {
        std::string placeholder;
        std::vector<size_t> positions;  // 存储所有出现位置
    };

    static std::expected<FKEmailTemplate, FileError> create(
        const std::filesystem::path& template_path,
        const std::vector<std::string>& placeholders)
    {
        auto file_content = ReadFile(template_path);
        if (!file_content) {
            return std::unexpected(file_content.error());
        }

        std::string template_data(file_content->begin(), file_content->end());

        // 存储每个占位符的所有位置
        std::vector<ReplacementInfo> replacements;
        for (const auto& placeholder : placeholders) {
            if (placeholder.empty()) {
                return std::unexpected(FileError{
                    FileErrorCode::ReadFailure,
                    template_path,
                    "Empty placeholder encountered"
                    });
            }

            std::vector<size_t> positions;
            size_t pos = 0;

            // 查找所有出现位置
            while ((pos = template_data.find(placeholder, pos)) != std::string::npos) {
                positions.push_back(pos);
                pos += placeholder.length();  // 跳过当前占位符继续查找
            }

            if (positions.empty()) {
                return std::unexpected(FileError{
                    FileErrorCode::ReadFailure,
                    template_path,
                    "Placeholder not found in template: " + placeholder
                    });
            }

            // 按位置升序排序，方便后续替换
            //std::sort(positions.begin(), positions.end());
            replacements.push_back({ placeholder, std::move(positions) });
        }

        return FKEmailTemplate(std::move(template_data), std::move(replacements));
    }

    std::expected<std::string, TemplateError> generate(const std::vector<std::string>& values) const {
        if (values.size() != _replacements.size()) {
            return std::unexpected(TemplateError::InvalidValueCount);
        }
        if (_replacements.empty()) {
            return std::unexpected(TemplateError::PlacehlderNoInit);
        }

        std::string result = _template_data;

        // 创建替换位置列表（位置+索引），用于从后向前替换
        std::vector<std::pair<size_t, size_t>> all_positions;
        for (size_t i = 0; i < _replacements.size(); ++i) {
            for (size_t pos : _replacements[i].positions) {
                all_positions.emplace_back(pos, i);
            }
        }

        // 按位置降序排序（从后向前替换的关键）
        std::sort(all_positions.begin(), all_positions.end(),
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            }
        );

        // 执行替换（从后向前）
        for (const auto& [pos, idx] : all_positions) {
            const std::string& placeholder = _replacements[idx].placeholder;
            const std::string& value = values[idx];
            result.replace(pos, placeholder.length(), value);
        }

        return result;
    }

private:
    FKEmailTemplate(std::string template_data, std::vector<ReplacementInfo> replacements)
        : _template_data(std::move(template_data)),
        _replacements(std::move(replacements)) {
    }

    std::string _template_data;
    std::vector<ReplacementInfo> _replacements;  // 使用新结构存储
};

#endif // FK_EMAIL_TEMPLATE_H_