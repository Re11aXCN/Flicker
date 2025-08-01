#include "TimeTestMapper.h"

#include <magic_enum/magic_enum.hpp>

TimeTestMapper::TimeTestMapper() : BaseMapper<TimeTestEntity, std::uint32_t>() {

}

constexpr std::string TimeTestMapper::getTableName() const {
    return "test_time";
}

constexpr std::string TimeTestMapper::findByIdQuery() const {
    return "SELECT id, year, "
        "UNIX_TIMESTAMP(timestamp) * 1000 as create_time_ms, "
        "UNIX_TIMESTAMP(datetime) * 1000 as update_time_ms, "
        "date, time "
        "FROM " + getTableName() + " WHERE id = ?";
}

constexpr std::string TimeTestMapper::findAllQuery() const {
    return "SELECT * FROM " + getTableName() + " ORDER BY id";
}

constexpr std::string TimeTestMapper::insertQuery() const {
    return "INSERT INTO " + getTableName() +
        " year, datetime, date, time "
        "VALUES (?, ?, ?, ?)";
}

constexpr std::string TimeTestMapper::deleteByIdQuery() const {
    return "DELETE FROM " + getTableName() + " WHERE id = ?";
}

void TimeTestMapper::bindInsertParams(MySQLStmtPtr& stmtPtr, const TimeTestEntity& entity) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    /*year YEAR NOT NULL,
    datetime DATETIME(3) NOT NULL,
    date DATE NOT NULL,
    time TIME(3) NOT NULL*/

    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = const_cast<uint32_t*>(&entity._year);
    bind[0].buffer_length = sizeof(entity._year);
    bind[0].is_unsigned = 1;
    bind[0].length = 0;

    std::string time_str = utils::time_point_to_str(entity._datetime);
    unsigned long time_length = (unsigned long)time_str.length();
    bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[1].buffer = const_cast<char*>(time_str.c_str());
    bind[1].buffer_length = time_length + 1;
    bind[1].length = &time_length;

    bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer = const_cast<char*>(entity._date.c_str());
    bind[2].buffer_length = entity._date_length + 1;
    bind[2].length = const_cast<unsigned long*>(&entity._date_length);

    bind[3].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[3].buffer = const_cast<char*>(entity._time.c_str());
    bind[3].buffer_length = entity._time_length + 1;
    bind[3].length = const_cast<unsigned long*>(&entity._time_length);

    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

TimeTestEntity TimeTestMapper::createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
    char* isNulls, size_t columnCount) const
{
    using tp = std::chrono::system_clock::time_point;
    return TimeTestEntity{
        _parser.getValue<std::uint32_t>(&binds[0], lengths[0], isNulls[0], fields[0].type).value(),
        _parser.getValue<std::uint32_t>(&binds[1], lengths[1], isNulls[1], fields[1].type).value(),
        _parser.getValue<tp>(&binds[2], lengths[2], isNulls[2], fields[2].type).value(),
        _parser.getValue<tp>(&binds[3], lengths[3], isNulls[3], fields[3].type).value(),
        _parser.getValue<std::string>(&binds[4], lengths[4], isNulls[4], fields[4].type).value(),
        _parser.getValue<std::string>(&binds[5], lengths[5], isNulls[5], fields[5].type).value(),
    };
}

TimeTestEntity TimeTestMapper::createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields, unsigned long* lengths, size_t columnCount) const
{
    using tp = std::chrono::system_clock::time_point;
    return TimeTestEntity{
        _parser.getValue<std::uint32_t>(row, 0, lengths, false,  fields[0].type).value(),
        _parser.getValue<std::uint32_t>(row, 1, lengths, false,  fields[1].type).value(),
        _parser.getValue<tp>(row, 2, lengths, false,  fields[2].type).value(),
        _parser.getValue<tp>(row, 3, lengths, false,  fields[3].type).value(),
        _parser.getValue<std::string>(row, 4, lengths, false,  fields[4].type).value(),
        _parser.getValue<std::string>(row, 5, lengths, false,  fields[5].type).value(),
    };
}

void TimeTestMapper::testTimeType()
{
    try {
        {
            auto condition = QueryConditionBuilder::true_();
            auto results = queryEntitiesByCondition<>(condition);
            LOGGER_INFO(std::format("查询test_time数据总数: {}", results.size()));
            LOGGER_INFO("————实体数据: \n");
            for (const auto& entity : results) {
                LOGGER_INFO(std::format("{}\n", entity.toString()));
            }
        }
    }
    catch (const std::exception& e) {
        LOGGER_ERROR(std::format("等值条件测试失败: {}", e.what()));
    }
}
