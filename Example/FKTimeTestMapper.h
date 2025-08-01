/*************************************************************************************
 *
 * @ Filename     : TimeTestMapper.h
 * @ Description : 测试实体数据库映射器
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
#ifndef TIME_TEST_MAPPER_H_
#define TIME_TEST_MAPPER_H_

#include "Common/mysql/mapper/base_mapper.hpp"
#include "TimeTestEntity.h"
/*
* @note 
*       测试数据库的时间类型
*       year YEAR NOT NULL,
*       timestamp TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3),
*       datetime DATETIME(3) NOT NULL,
*       date DATE NOT NULL,
*       time TIME(3) NOT NULL
* 
*       经过测试，和"Impl-BaseMapper.ipp"的
*       MYSQL_FOREACH_BIND_RESSET 输出绑定的case分支定义，
*       以下数据库在内存视图类型映射C++类型：
*       MYSQL_TYPE_YEAR——uint32_t
*       MYSQL_TYPE_TIMESTAMP——MYSQL_TIME
*       MYSQL_TYPE_DATETIME——MYSQL_TIME
*       MYSQL_TYPE_DATE——std::string
*       MYSQL_TYPE_TIME——std::string
*       
*       结果是能够成功的读取内存视图类型的数据，并转换为C++类型，构造TimeTestEntity实体对象。
*/

class TimeTestMapper : public BaseMapper<TimeTestEntity, std::uint32_t> {
public:
    TimeTestMapper();
    ~TimeTestMapper() override = default;

    void testTimeType();

protected:
    constexpr std::string getTableName() const override;
    constexpr std::string findByIdQuery() const override;
    constexpr std::string findAllQuery() const override;
    constexpr std::string insertQuery() const override;
    constexpr std::string deleteByIdQuery() const override;

    void bindInsertParams(MySQLStmtPtr& stmtPtr, const TimeTestEntity& entity) const override;

    TimeTestEntity createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
        char* isNulls, size_t columnCount) const override;
    TimeTestEntity createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields,
        unsigned long* lengths, size_t columnCount) const override;
};

#endif // !TIME_TEST_MAPPER_H_