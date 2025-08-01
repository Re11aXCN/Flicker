#include "Common/mysql/mysql_connection_pool.h"
#include "UserTestMapper.h"
#include "TimeTestMapper.h"
inline int TEST_MAPPER_FUNC()
{
    try
    {
        bool ok = Logger::getInstance().initialize("Flicker-Server", Logger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;
        /*UserTestMapper testMapper;*/
        TimeTestMapper testTimeMapper;
        MySQLConnectionPool::getInstance()->executeWithConnection([](MYSQL* mysql) {
            try {
                // 创建测试表SQL - 与users表结构相同
                std::string createTestTableSQL = R"(
CREATE TABLE IF NOT EXISTS
test_users (
id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
uuid VARCHAR(36) NOT NULL UNIQUE DEFAULT (UUID()),
username VARCHAR(30) NOT NULL UNIQUE,
email VARCHAR(320) NOT NULL UNIQUE,
password VARCHAR(60) NOT NULL, 
create_time TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3),
update_time TIMESTAMP(3) DEFAULT NULL,
INDEX idx_email (email),
INDEX idx_username (username)
)
ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
)";
                if (mysql_query(mysql, createTestTableSQL.c_str())) {
                    LOGGER_ERROR(std::format("创建测试表失败: {}", mysql_error(mysql)));
                    return false;
                }

                std::string createTestTimeTableSQL = R"(
CREATE TABLE IF NOT EXISTS test_time (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  year YEAR NOT NULL,
  timestamp TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3),
  datetime DATETIME(3) NOT NULL,
  date DATE NOT NULL,
  time TIME(3) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
)";
                if (mysql_query(mysql, createTestTimeTableSQL.c_str())) {
                    LOGGER_ERROR(std::format("创建测试表失败: {}", mysql_error(mysql)));
                    return false;
                }

                // 清空测试表数据
                if (mysql_query(mysql, "TRUNCATE TABLE test_users;")) {
                    LOGGER_ERROR(std::format("清空测试表失败: {}", mysql_error(mysql)));
                    return false;
                }
                // 清空测试时间表数据
                if (mysql_query(mysql, "TRUNCATE TABLE test_time;")) {
                    LOGGER_ERROR(std::format("清空测试表失败: {}", mysql_error(mysql)));
                    return false;
                }

                // 插入测试数据
                std::string insertSQL = R"(
INSERT INTO test_users (username, email, password, create_time, update_time) VALUES
('test1', 'test1@test.com', 'password1', CURRENT_TIMESTAMP(3), NULL),
('test2', 'test2@test.com', 'password2', CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3)),
('test3', 'test3@test.com', 'password3', CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3)),
('test4', 'test4@test.com', 'password4', CURRENT_TIMESTAMP(3), NULL),
('test5', 'test5@test.com', 'password5', CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3));
)";
                if (mysql_query(mysql, insertSQL.c_str())) {
                    LOGGER_ERROR(std::format("插入测试数据失败: {}", mysql_error(mysql)));
                    return false;
                }

                std::string insertTimeSQL = R"(
INSERT INTO `test_time` (`year`, `timestamp`, `datetime`, `date`, `time`)
VALUES
  (2025, '2025-07-31 08:45:30.123', '2025-07-31 08:45:30.123', '2025-07-31', '08:45:30.123'),
  (2020, '2020-01-15 14:30:00.500', '2020-01-15 14:30:00.500', '2020-01-15', '14:30:00.500'),
  (2030, '2030-11-20 20:15:45.789', '2030-11-20 20:15:45.789', '2030-11-20', '20:15:45.789'),
  (2015, '2015-05-01 00:00:00.000', '2015-05-01 00:00:00.000', '2015-05-01', '00:00:00.000'),
  (2023, '2023-12-25 23:59:59.999', '2023-12-25 23:59:59.999', '2023-12-25', '23:59:59.999');
)";
                if (mysql_query(mysql, insertTimeSQL.c_str())) {
                    LOGGER_ERROR(std::format("插入测试数据失败: {}", mysql_error(mysql)));
                    return false;
                }

                LOGGER_INFO("测试表创建并插入数据成功");
                return true;
            }
            catch (const std::exception& e) {
                LOGGER_ERROR(std::format("创建表异常: {}", e.what()));
                return false;
            }
            });

        // 创建测试Mapper并执行测试
        LOGGER_INFO("开始执行查询条件测试...");
        testTimeMapper.testTimeType();
        //testMapper.testTrueCondition();
        
        //// 测试等值条件
        //testMapper.testEqualCondition();

        //// 测试不等值条件
        //testMapper.testNotEqualCondition();

        //// 测试范围条件
        //testMapper.testBetweenCondition();

        //// 测试大于/大于等于条件
        //testMapper.testGreaterThanCondition();

        //// 测试小于/小于等于条件
        //testMapper.testLessThanCondition();

        //// 测试模糊匹配条件
        //testMapper.testLikeCondition();

        //// 测试正则表达式条件
        //testMapper.testRegexpCondition();
        //
        //// 测试IN条件
        //testMapper.testInCondition();

        //// 测试NOT IN条件
        //testMapper.testNotInCondition();

        //// 测试IS NULL/IS NOT NULL条件
        //testMapper.testIsNullCondition();

        //// 测试AND条件
        //testMapper.testAndCondition();
        //
        //// 测试OR条件
        //testMapper.testOrCondition();

        //// 测试NOT条件
        //testMapper.testNotCondition();

        //// 测试原始SQL条件
        //testMapper.testRawCondition();

        //// 测试复杂查询条件
        //testMapper.testComplexCondition();

        //// 测试更新操作
        //testMapper.testUpdateByCondition();

        //// 测试删除操作
        //testMapper.testDeleteByCondition();
        

        LOGGER_INFO("查询条件测试完成");

        Logger::getInstance().shutdown();
    }
    catch (const std::exception& e)
    {
        LOGGER_ERROR(std::format("异常: {}", e.what()));
        Logger::getInstance().shutdown();
        return 1;
    }
    return 0;
}
#include <utils.h>
#include <print>
#include <iostream>
void Test_URL_D_Encode() {
    // 原始字符串（包含中文和特殊字符）
    std::string text = "http://www.baidu.com/s?ie=utf-8&f=8&tn=baidu&wd=临时邮箱";

    // URL 编码
    std::string encoded = Utils::url_encode(text);
    std::println("编码后:\n{}\n\n", encoded);

    // URL 解码
    std::string decoded = Utils::url_decode(encoded);
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
    auto params = Utils::parse_query_params(uri, url);
    std::println("url is : {}\n", url);
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
        auto result = Utils::join_range(separator, fieldNames);
        // 防止编译器优化掉结果
        if (result.empty()) std::cout << "Error";
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();

    // 测试普通循环拼接性能
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = Utils::plain_join_range(separator, fieldNames);
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
    std::cout << "join_range: " << Utils::join_range(", ", test) << "\n";
    std::cout << "naive_join: " << Utils::plain_join_range(", ", test) << "\n";
}