﻿/*************************************************************************************
 *
 * @ Filename     : TimeTestEntity.h
 * @ Description : 测试实体类，用于测试查询条件功能
 *
 * @ Version     : V1.0
 * @ Author      : Re11a
 * @ Date Created: 2025/7/28
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By:
 * Modifications:
 * ======================================
*************************************************************************************/
#ifndef TEST_ENTITY_H_
#define TEST_ENTITY_H_

#include "Common/utils/utils.h"
#include "Common/mysql/mapper/field_mapper.hpp"
#include "Common/mysql/entity/base_entity.h"
class TimeTestMapper;
class TimeTestEntity : public BaseEntity {
    friend class TimeTestMapper;
public:
    using FieldTypeList = TypeList<
        std::uint32_t,                                       
        std::uint32_t,                                       
        std::chrono::system_clock::time_point,               
        std::chrono::system_clock::time_point,               
        std::string,                                         
        std::string
    >;
    static constexpr std::array<const char*, 6> FIELD_NAMES = {
       "id", "year", "timestamp", "datetime", "date", "time"
    };

    explicit TimeTestEntity() = default;
    ~TimeTestEntity() = default;

    TimeTestEntity(std::uint32_t id, std::uint32_t year, 
        std::chrono::system_clock::time_point timestamp,
        std::chrono::system_clock::time_point datetime,
        const std::string& date, const std::string& time)
        : _id(id), _year(year), _timestamp(timestamp), _datetime(datetime),
        _date(date), _time(time) 
    {
        _date_length = (unsigned long)date.length();
        _time_length = (unsigned long)time.length();
    }

    // 转换为字符串表示
    std::vector<std::string> getFieldNames() const override {
        return { FIELD_NAMES.begin(), FIELD_NAMES.end() };
    }

    std::vector<std::string> getFieldValues() const override {
        return { std::to_string(_id), std::to_string(_year), 
            utils::time_point_to_str(_timestamp),
            utils::time_point_to_str(_datetime),
            _date,
            _time
        };
    }

    std::uint32_t _id{ 0 };
    std::uint32_t _year{ 0 };
    std::chrono::system_clock::time_point _timestamp;
    std::chrono::system_clock::time_point _datetime;
    std::string _date;
    std::string _time;
    unsigned long _date_length{ 0 };
    unsigned long _time_length{ 0 };
};


#endif // !TEST_ENTITY_H_