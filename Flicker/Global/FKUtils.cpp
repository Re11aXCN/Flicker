#include "FKUtils.h"
#include <print>
#include <iostream>
void Test_URL_D_Encode() {
    // 原始字符串（包含中文和特殊字符）
    std::string text = "http://www.baidu.com/s?ie=utf-8&f=8&tn=baidu&wd=临时邮箱";

    // URL 编码
    std::string encoded = FKUtils::url_encode(text);
    std::println("编码后:\n{}\n\n", encoded);

    // URL 解码
    std::string decoded = FKUtils::url_decode(encoded);
    std::cout << "-------------------------------------------------\n";
    std::println("解码后:\n{}\n\n", decoded);

}

void Test_Http_Get_Query_Params() {
    // 带查询参数的 URL
    std::string_view uri =
        "https://www.example.com/search"
        "?q=C%2B%2B+Programming"
        "&lang=zh-CN"
        "&price=29.99"
        "&discount=50%25"
        "&filter=new"
        "&filter=popular"
        "&empty_value"
        "&special_chars=%21%40%23%24%25%5E%26*";

    // 解析查询参数
    std::string url;
    auto params = FKUtils::parse_query_params(uri, url);
    std::println("url is : {}\n", url) ;
    // 打印解析结果
    std::cout << "解析结果:\n";
    for (const auto& [key, value] : params) {
        std::cout << "  " << key << " = " << value << "\n";
    }

}


// 性能测试函数
// 拼接3个字符串，迭代1次，naive_join 快 100.00% 0us, 2us
// 拼接5个字符串，迭代20次，性能一样
// 拼接10个字符串，迭代1次，性能一样
// 拼接10个字符串，迭代10次，jojo_range 快 40.00%
/*
拼接3个字符串 迭代1000次
join_range 耗时 : 436 μs(0.44 ms)
普通循环耗时 : 406 μs(0.41 ms)
性能差异 : 普通循环快 6.88 %
*/
void performance_test() {
    // 创建包含10个字符串的vector
    std::vector<std::string> fieldNames;
    for (int i = 0; i < 3; ++i) {
        fieldNames.push_back("field" + std::to_string(i + 1));
    }

    const std::string separator = ", ";
    const int iterations = 1000;

    std::cout << "性能测试: " << iterations << "次迭代，10个字符串拼接\n";
    std::cout << "===========================================\n";

    // 测试 join_range 性能
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = FKUtils::join_range(separator, fieldNames);
        // 防止编译器优化掉结果
        if (result.empty()) std::cout << "Error";
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();

    // 测试普通循环拼接性能
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = FKUtils::plain_join_range(separator, fieldNames);
        // 防止编译器优化掉结果
        if (result.empty()) std::cout << "Error";
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();

    // 计算性能差异百分比
    double diff = static_cast<double>(duration2 - duration1) / duration1 * 100.0;

    // 输出结果
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "join_range 耗时: " << duration1 << " μs (" << (duration1 / 1000.0) << " ms)\n";
    std::cout << "普通循环耗时: " << duration2 << " μs (" << (duration2 / 1000.0) << " ms)\n";
    std::cout << "性能差异: ";

    if (duration1 < duration2) {
        std::cout << "join_range 快 " << diff << "%\n";
    }
    else {
        std::cout << "普通循环快 " << -diff << "%\n";
    }
}

void function_test() {
    // 验证功能正确性
    std::vector<std::string> test = { "A", "B", "C" };
    std::cout << "\n功能测试:\n";
    std::cout << "join_range: " << FKUtils::join_range(", ", test) << "\n";
    std::cout << "naive_join: " << FKUtils::plain_join_range(", ", test) << "\n";
}