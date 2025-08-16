#ifndef UNIVERSAL_MYSQL_TIME_H_
#define UNIVERSAL_MYSQL_TIME_H_
#include <mysql.h>
#include <chrono>
#include <ctime>
#include <stdexcept>
namespace universal::mysql::time {
    namespace time {

    }
    inline MYSQL_TIME mysqlCurrentTime()
    {
        MYSQL_TIME value;
        memset(&value, 0, sizeof(value));
        auto now = std::chrono::system_clock::now();
        auto createTimeT = std::chrono::system_clock::to_time_t(now);
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

        // 获取 UTC 时间（TIMESTAMP 类型需要 UTC）
        struct tm timeinfo;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&timeinfo, &createTimeT);
#else
        localtime_r(&createTimeT, &timeinfo);
#endif
        // 填充 MYSQL_TIME 结构
        value.year = timeinfo.tm_year + 1900;
        value.month = timeinfo.tm_mon + 1;
        value.day = timeinfo.tm_mday;
        value.hour = timeinfo.tm_hour;
        value.minute = timeinfo.tm_min;
        value.second = timeinfo.tm_sec;
        value.second_part = (unsigned long)millis * 1000;  // 毫秒转微秒
        value.time_type = MYSQL_TIMESTAMP_DATETIME;

        return value;
    }

    inline std::chrono::system_clock::time_point mysqlTimeToChrono(const MYSQL_TIME& mysqlTime) {
        std::chrono::system_clock::time_point timePoint;

        switch (mysqlTime.time_type) {
        case MYSQL_TIMESTAMP_ERROR:
            throw std::runtime_error("Invalid MYSQL_TIME");
        case MYSQL_TIMESTAMP_DATE:
        case MYSQL_TIMESTAMP_DATETIME: {
            // 创建并填充 tm 结构
            std::tm tm = {};
            tm.tm_year = mysqlTime.year - 1900;
            tm.tm_mon = mysqlTime.month - 1;    // 月份从 0 开始
            tm.tm_mday = mysqlTime.day;
            tm.tm_hour = mysqlTime.hour;
            tm.tm_min = mysqlTime.minute;
            tm.tm_sec = mysqlTime.second;
            tm.tm_isdst = -1;  // 自动判断夏令时

            // 转换为 time_t
            std::time_t t = std::mktime(&tm);
            if (t == -1) {
                throw std::runtime_error("Failed to convert date/time");
            }
            timePoint = std::chrono::system_clock::from_time_t(t);

            // 为 TIMESTAMP_DATETIME 类型添加微秒
            if (mysqlTime.time_type == MYSQL_TIMESTAMP_DATETIME) {
                timePoint += std::chrono::microseconds(mysqlTime.second_part);
            }
            break;
        }
        case MYSQL_TIMESTAMP_TIME:
            // 时间类型（无日期）无法转换为绝对时间点
            throw std::runtime_error("MYSQL_TIMESTAMP_TIME not supported for absolute time conversion");
        case MYSQL_TIMESTAMP_DATETIME_TZ:
            // 时区信息在标准 MySQL API 中不可用
            throw std::runtime_error("MYSQL_TIMESTAMP_DATETIME_TZ requires time zone handling which is not implemented");
        case MYSQL_TIMESTAMP_NONE:
            throw std::runtime_error("MYSQL_TIMESTAMP_NONE is not a valid timestamp");
        default:
            throw std::runtime_error("Unknown MYSQL_TIME type");
        }

        return timePoint;
    }
};

#endif // UNIVERSAL_MYSQL_TIME_H_