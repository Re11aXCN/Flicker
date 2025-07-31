#include "FKUserTestMapper.h"

#include <magic_enum/magic_enum.hpp>

FKUserTestMapper::FKUserTestMapper() : FKBaseMapper<FKUserTestEntity, std::uint32_t>() {

}

constexpr std::string FKUserTestMapper::getTableName() const {
    return "test_users";
}

constexpr std::string FKUserTestMapper::findByIdQuery() const {
    return "SELECT id, uuid, username, email, password, "
           "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
           "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
           "FROM " + getTableName() + " WHERE id = ?";
}

constexpr std::string FKUserTestMapper::findAllQuery() const {
    return "SELECT id, uuid, username, email, password, "
           "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
           "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
           "FROM " + getTableName() + " ORDER BY id";
}

constexpr std::string FKUserTestMapper::findByEmailQuery() const {
    return "SELECT id, uuid, username, email, password, "
        "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
        "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
        "FROM " + getTableName() + " WHERE email = ?";
}

constexpr std::string FKUserTestMapper::findByUsernameQuery() const {
    return "SELECT id, uuid, username, email, password, "
        "UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms, "
        "UNIX_TIMESTAMP(update_time) * 1000 as update_time_ms "
        "FROM " + getTableName() + " WHERE username = ?";
}

constexpr std::string FKUserTestMapper::insertQuery() const {
    return "INSERT INTO " + getTableName() + 
           " (username, email, password) "
           "VALUES (?, ?, ?)";
}

constexpr std::string FKUserTestMapper::deleteByIdQuery() const {
    return "DELETE FROM " + getTableName() + " WHERE id = ?";
}

void FKUserTestMapper::bindInsertParams(MySQLStmtPtr& stmtPtr, const FKUserTestEntity& entity) const {
    MYSQL_STMT* stmt = stmtPtr;
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));

    // Username
    auto* usernameLength = const_cast<unsigned long*>(entity.getUsernameLength());
    const std::string& username = entity.getUsername();
    bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = *usernameLength + 1;
    bind[0].length = usernameLength;
    
    // Email
    auto* emailLength = const_cast<unsigned long*>(entity.getEmailLength());
    const std::string& email = entity.getEmail();
    bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[1].buffer = const_cast<char*>(entity.getEmail().c_str());
    bind[1].buffer_length = *emailLength + 1;
    bind[1].length = emailLength;

    // Password
    auto* passwordLength = const_cast<unsigned long*>(entity.getPasswordLength());
    const std::string& password = entity.getPassword();;
    bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind[2].buffer = const_cast<char*>(entity.getPassword().c_str());
    bind[2].buffer_length = *passwordLength + 1;
    bind[2].length = passwordLength;
  
    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string error = mysql_stmt_error(stmt);
        throw DatabaseException("Bind param failed: " + error);
    }
}

FKUserTestEntity FKUserTestMapper::createEntityFromBinds(MYSQL_BIND* binds, MYSQL_FIELD* fields, unsigned long* lengths,
    char* isNulls, size_t columnCount) const
{
    using tp = std::chrono::system_clock::time_point;
    if (columnCount < 7) {
        throw DatabaseException("Insufficient columns in result set");
    }
    return FKUserTestEntity{
        _parser.getValue<std::uint32_t>(&binds[0], lengths[0], isNulls[0], fields[0].type).value(),
        _parser.getValue<std::string>(&binds[1], lengths[1], isNulls[1], fields[1].type).value(),
        _parser.getValue<std::string>(&binds[2], lengths[2], isNulls[2], fields[2].type).value(),
        _parser.getValue<std::string>(&binds[3], lengths[3], isNulls[3], fields[3].type).value(),
        _parser.getValue<std::string>(&binds[4], lengths[4], isNulls[4], fields[4].type).value(),
        _parser.getValue<tp>(&binds[5], lengths[5], isNulls[5], fields[5].type).value(),
        _parser.getValue<tp>(&binds[6], lengths[6], isNulls[6], fields[6].type)
    };
}

FKUserTestEntity FKUserTestMapper::createEntityFromRow(MYSQL_ROW row, MYSQL_FIELD* fields, unsigned long* lengths, size_t columnCount) const
{
    using tp = std::chrono::system_clock::time_point;
    if (columnCount < 7) {
        throw DatabaseException("Insufficient columns in recreateEntityFromRowsult set");
    }
    return FKUserTestEntity{
        _parser.getValue<std::uint32_t>(row, 0, lengths, false, fields[0].type).value(),
        _parser.getValue<std::string>(row, 1, lengths, false, fields[1].type).value(),
        _parser.getValue<std::string>(row, 2, lengths, false, fields[2].type).value(),
        _parser.getValue<std::string>(row, 3, lengths, false, fields[3].type).value(),
        _parser.getValue<std::string>(row, 4, lengths, false, fields[4].type).value(),
        _parser.getValue<tp>(row, 5, lengths, false, fields[5].type).value(),
        _parser.getValue<tp>(row, 6, lengths, ResultSetParser::isFieldNull(row, 6), fields[6].type)
    };
}

std::optional<FKUserTestEntity> FKUserTestMapper::findByEmail(const std::string& email) {
    try {
        auto results = queryEntities<>(findByEmailQuery(), varchar{ email.data(), static_cast<unsigned long>(email.length()) });
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        return results.front();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByEmail error: {}", e.what()));
        throw;
    }
}

std::optional<FKUserTestEntity> FKUserTestMapper::findByUsername(const std::string& username) {
    try {
        auto results = queryEntities<>(findByUsernameQuery(), varchar{ username.data(), static_cast<unsigned long>(username.length()) });
        
        if (results.empty()) {
            return std::nullopt;
        }
        
        return results.front();
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("findByUsername error: {}", e.what()));
        throw;
    }
}

void FKUserTestMapper::testTrueCondition()
{
    try {
        auto condition = QueryConditionBuilder::true_();

        {
            auto results1 = queryEntitiesByCondition<>(condition);
            FK_SERVER_INFO(std::format("TrueCondition测试 - queryEntitiesByCondition查询结果数量: {}", results1.size()));
            for (const auto& entity : results1) {
                FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
            }
        }
        {
            std::vector<std::string> fields{ "username", "email", "password" };
            auto results2 = queryFieldsByCondition<>(condition, fields);
            FK_SERVER_INFO(std::format("TrueCondition测试 - queryFieldsByCondition查询结果数量: {}", results2.size()));
            for (const auto& entity : results2) {
                for (const auto& field : entity) {
                    FK_SERVER_INFO(std::format("  字段: {} 值: {}", field.first, getFieldMapValueOrDefault(entity, field.first, std::string{})));
                }
            }
        }
        {
            auto results3 = countByCondition(condition);
            FK_SERVER_INFO(std::format("TrueCondition测试 - countByCondition查询结果数量: {}", results3));
        }
        {
            std::string f1 = "123456789";
            auto results4 = updateFieldsByCondition(condition, 
                std::vector<std::string>{"password"},
                BindableParam{ varchar{ f1.data(), static_cast<unsigned long>(f1.length()) } });

            FK_SERVER_INFO(std::format("TrueCondition测试 - updateFieldsByCondition影响行数: {}", results4.second));
            auto results1 = queryEntitiesByCondition<>(condition);
            FK_SERVER_INFO(std::format("TrueCondition测试 - queryEntitiesByCondition查询结果数量: {}", results1.size()));
            for (const auto& entity : results1) {
                FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
            }
        }
        {
            auto results5 = deleteByCondition(condition);
            FK_SERVER_INFO(std::format("TrueCondition测试 - deleteByCondition影响行数: {}", results5.second));
        }
        {
            auto results6 = queryFieldsByCondition<>(condition, {"UNIX_TIMESTAMP(create_time) * 1000 as create_time_ms"});
            FK_SERVER_INFO(std::format("UNIX_TIMESTAMP 时间测试: {}", results6.size()));
            for (const auto& entity : results6) {
                for (const auto& field : entity) {
                    auto value = getFieldMapValueOrDefault(entity, field.first, double{});
                    int64_t milliseconds = static_cast<int64_t>(value);
                    auto tp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(milliseconds)
                    );

                    FK_SERVER_INFO(std::format("字段: {} double值: {} time_point值: {}",
                        field.first,
                        value,
                        FKUtils::time_point_to_str(tp)));

                }
            }
        }
    }
    catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("等值条件测试失败: {}", e.what()));
    }
}
/**/
// 测试等值条件
void FKUserTestMapper::testEqualCondition() {
    try {
        // 创建等值条件
        auto condition = QueryConditionBuilder::eq_("username", varchar{"test1", 5});
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("等值条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("等值条件测试失败: {}", e.what()));
    }
}

// 测试不等值条件
void FKUserTestMapper::testNotEqualCondition() {
    try {
        // 创建不等值条件
        auto condition = QueryConditionBuilder::neq_("username", varchar{"test1", 5});
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("不等值条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("不等值条件测试失败: {}", e.what()));
    }
}

// 测试范围条件
void FKUserTestMapper::testBetweenCondition() {
    try {
        // 创建范围条件 (id 在 1 到 3 之间)
        auto condition = QueryConditionBuilder::between_("id", (uint32_t)1, (uint32_t)3);
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("范围条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("范围条件测试失败: {}", e.what()));
    }
}

// 测试大于条件
void FKUserTestMapper::testGreaterThanCondition() {
    try {
        // 创建大于条件 (id > 2)
        auto condition = QueryConditionBuilder::gt_("id", (uint32_t)2);
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("大于条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
        
        // 创建大于等于条件 (id >= 2)
        auto conditionGE = QueryConditionBuilder::ge_("id", (uint32_t)2);
        
        // 查询符合条件的实体
        auto resultsGE = queryEntitiesByCondition<>(conditionGE);
        
        // 输出结果
        FK_SERVER_INFO(std::format("大于等于条件测试 - 查询结果数量: {}", resultsGE.size()));
        for (const auto& entity : resultsGE) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("大于条件测试失败: {}", e.what()));
    }
}

// 测试小于条件
void FKUserTestMapper::testLessThanCondition() {
    try {
        // 创建小于条件 (id < 3)
        auto condition = QueryConditionBuilder::lt_("id", (uint32_t)3);
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("小于条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
        
        // 创建小于等于条件 (id <= 3)
        auto conditionLE = QueryConditionBuilder::le_("id", (uint32_t)3);
        
        // 查询符合条件的实体
        auto resultsLE = queryEntitiesByCondition<>(conditionLE);
        
        // 输出结果
        FK_SERVER_INFO(std::format("小于等于条件测试 - 查询结果数量: {}", resultsLE.size()));
        for (const auto& entity : resultsLE) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("小于条件测试失败: {}", e.what()));
    }
}

// 测试模糊匹配条件
void FKUserTestMapper::testLikeCondition() {
    try {
        // 创建模糊匹配条件 (username LIKE 'test%')
        auto condition = QueryConditionBuilder::like_("username", varchar{ "test%", 5 });
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("模糊匹配条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("模糊匹配条件测试失败: {}", e.what()));
    }
}

// 测试正则表达式条件
void FKUserTestMapper::testRegexpCondition() {
    try {
        // 创建正则表达式条件 (username REGEXP '^test[0-9]')
        auto condition = QueryConditionBuilder::regexp_("username", varchar{ "^test[0-9]", 10 });
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("正则表达式条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("正则表达式条件测试失败: {}", e.what()));
    }
}

// 测试IN条件
void FKUserTestMapper::testInCondition() {
    try {
        // 创建IN条件 (id IN (1, 3, 5))
        std::vector<uint32_t> ids = {1, 3, 5};
        auto condition = QueryConditionBuilder::in_("id", ids);
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("IN条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("IN条件测试失败: {}", e.what()));
    }
}

// 测试NOT IN条件
void FKUserTestMapper::testNotInCondition() {
    try {
        // 创建NOT IN条件 (id NOT IN (2, 4, 6))
        std::vector<uint32_t> ids = {2, 4, 6};
        auto condition = QueryConditionBuilder::notIn_("id", ids);
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("NOT IN条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("NOT IN条件测试失败: {}", e.what()));
    }
}

// 测试IS NULL条件
void FKUserTestMapper::testIsNullCondition() {
    try {
        // 创建IS NULL条件 (update_time IS NULL)
        auto condition = QueryConditionBuilder::isNull_("update_time");
        
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("IS NULL条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
        
        // 创建IS NOT NULL条件 (update_time IS NOT NULL)
        auto conditionNotNull = QueryConditionBuilder::isNotNull_("update_time");
        
        // 查询符合条件的实体
        auto resultsNotNull = queryEntitiesByCondition<>(conditionNotNull);
        
        // 输出结果
        FK_SERVER_INFO(std::format("IS NOT NULL条件测试 - 查询结果数量: {}", resultsNotNull.size()));
        for (const auto& entity : resultsNotNull) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("IS NULL条件测试失败: {}", e.what()));
    }
}

// 测试AND条件
void FKUserTestMapper::testAndCondition() {
    try {
        // 创建AND条件 (id > 1 AND username LIKE 'test%')
        auto andCondition = QueryConditionBuilder::and_();
        andCondition->addCondition(QueryConditionBuilder::gt_("id", (uint32_t)1));
        andCondition->addCondition(QueryConditionBuilder::like_("username", varchar{ "test%", 5 }));
        std::unique_ptr<IQueryCondition> baseCondition = std::move(andCondition);

        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(baseCondition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("AND条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("AND条件测试失败: {}", e.what()));
    }
}

// 测试OR条件
void FKUserTestMapper::testOrCondition() {
    try {
        // 创建OR条件 (id = 1 OR id = 3)
        auto orCondition = QueryConditionBuilder::or_();
        orCondition->addCondition(QueryConditionBuilder::eq_("id", (uint32_t)1));
        orCondition->addCondition(QueryConditionBuilder::eq_("id", (uint32_t)3));
        std::unique_ptr<IQueryCondition> baseCondition = std::move(orCondition);

        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(baseCondition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("OR条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("OR条件测试失败: {}", e.what()));
    }
}

// 测试NOT条件
void FKUserTestMapper::testNotCondition() {
    try {
        // 创建NOT条件 (NOT (id < 3))
        auto innerCondition = QueryConditionBuilder::lt_("id", (uint32_t)3);
        auto notCondition = QueryConditionBuilder::not_(std::move(innerCondition));
        std::unique_ptr<IQueryCondition> baseCondition = std::move(notCondition);

        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(baseCondition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("NOT条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("NOT条件测试失败: {}", e.what()));
    }
}

// 测试原始SQL条件
void FKUserTestMapper::testRawCondition() {
    try {
        // 创建原始SQL条件 (id % 2 = 0)
        auto condition = std::make_unique<RawCondition>("id % 2 = 0");
        std::unique_ptr<IQueryCondition> baseCondition = std::move(condition);

        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(baseCondition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("原始SQL条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("原始SQL条件测试失败: {}", e.what()));
    }
}

// 测试更新操作
void FKUserTestMapper::testUpdateByCondition() {
    try {
        // 创建条件 (id = 1)
        auto condition = QueryConditionBuilder::eq_("id", (uint32_t)1);
        
        // 更新字段
        std::vector<std::string> fieldNames = {"email", "password", "update_time"};
        auto [status, affectedRows] = updateFieldsByCondition(condition, fieldNames,
            ExpressionParam("'123@test.com'"),
            BindableParam{ varchar{"updated_password", 16} },
            ExpressionParam("NOW(3)"));
        
        // 输出结果
        FK_SERVER_INFO(std::format("更新操作测试 - 状态: {}", magic_enum::enum_name(status)));
        
        // 查询更新后的实体
        auto entity = findById(1);
        if (entity) {
            FK_SERVER_INFO(std::format("  更新后的实体: {}", entity->toString()));
            FK_SERVER_INFO(std::format("  新密码: {}", entity->getPassword()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("更新操作测试失败: {}", e.what()));
    }
}

// 测试删除操作
void FKUserTestMapper::testDeleteByCondition() {
    try {
        // 创建条件 (id = 5)
        auto condition = QueryConditionBuilder::eq_("id", (uint32_t)5);
        
        // 删除符合条件的实体
        auto [status, affectedRows] = deleteByCondition(condition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("删除操作测试 - 状态: {}", magic_enum::enum_name(status)));
        
        // 验证删除结果
        auto entity = findById(5);
        FK_SERVER_INFO(std::format("  ID=5的实体是否存在: {}", entity.has_value() ? "是" : "否"));
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("删除操作测试失败: {}", e.what()));
    }
}

// 测试复杂查询条件
void FKUserTestMapper::testComplexCondition() {
    try {
        // 创建复杂条件: (id > 1 AND (username LIKE 'test%' OR email LIKE '%@test.com')) AND update_time IS NOT NULL
        auto andCondition = std::make_unique<AndCondition>();
        andCondition->addCondition(QueryConditionBuilder::gt_("id", (uint32_t)1));
        
        auto orCondition = std::make_unique<OrCondition>();
        orCondition->addCondition(QueryConditionBuilder::like_("username", varchar{ "test%", 5 }));
        orCondition->addCondition(QueryConditionBuilder::like_("email", varchar{ "%@test.com", 10 }));
        
        andCondition->addCondition(std::move(orCondition));
        andCondition->addCondition(QueryConditionBuilder::isNotNull_("update_time"));
        
        std::unique_ptr<IQueryCondition> baseCondition = std::move(andCondition);
        // 查询符合条件的实体
        auto results = queryEntitiesByCondition<>(baseCondition);
        
        // 输出结果
        FK_SERVER_INFO(std::format("复杂条件测试 - 查询结果数量: {}", results.size()));
        for (const auto& entity : results) {
            FK_SERVER_INFO(std::format("  实体: {}", entity.toString()));
        }
    } catch (const std::exception& e) {
        FK_SERVER_ERROR(std::format("复杂条件测试失败: {}", e.what()));
    }
}
