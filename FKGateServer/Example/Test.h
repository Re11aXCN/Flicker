#include "MySQL/FKMySQLConnectionPool.h"
#include "FKUserTestMapper.h"
#include "FKTimeTestMapper.h"
inline int TEST_MAPPER_FUNC()
{
    try
    {
        bool ok = FKLogger::getInstance().initialize("Flicker-Server", FKLogger::SingleFile, true);
        if (!ok)  return EXIT_FAILURE;
        /*FKUserTestMapper testMapper;*/
        FKTimeTestMapper testTimeMapper;
        FKMySQLConnectionPool::getInstance()->executeWithConnection([](MYSQL* mysql) {
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
                    FK_SERVER_ERROR(std::format("创建测试表失败: {}", mysql_error(mysql)));
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
                    FK_SERVER_ERROR(std::format("创建测试表失败: {}", mysql_error(mysql)));
                    return false;
                }

                // 清空测试表数据
                if (mysql_query(mysql, "TRUNCATE TABLE test_users;")) {
                    FK_SERVER_ERROR(std::format("清空测试表失败: {}", mysql_error(mysql)));
                    return false;
                }
                // 清空测试时间表数据
                if (mysql_query(mysql, "TRUNCATE TABLE test_time;")) {
                    FK_SERVER_ERROR(std::format("清空测试表失败: {}", mysql_error(mysql)));
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
                    FK_SERVER_ERROR(std::format("插入测试数据失败: {}", mysql_error(mysql)));
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
                    FK_SERVER_ERROR(std::format("插入测试数据失败: {}", mysql_error(mysql)));
                    return false;
                }

                FK_SERVER_INFO("测试表创建并插入数据成功");
                return true;
            }
            catch (const std::exception& e) {
                FK_SERVER_ERROR(std::format("创建表异常: {}", e.what()));
                return false;
            }
            });

        // 创建测试Mapper并执行测试
        FK_SERVER_INFO("开始执行查询条件测试...");
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
        

        FK_SERVER_INFO("查询条件测试完成");

        FKLogger::getInstance().shutdown();
    }
    catch (const std::exception& e)
    {
        FK_SERVER_ERROR(std::format("异常: {}", e.what()));
        FKLogger::getInstance().shutdown();
        return 1;
    }
    return 0;
}