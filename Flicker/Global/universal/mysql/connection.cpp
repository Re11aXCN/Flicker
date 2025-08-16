#include "connection.h"

#include <stdexcept>
#include <format>
#include "Library/Logger/logger.h"

namespace universal::mysql {
    Connection::Connection(const ConnectionOptions& opts)
        : _opts(opts), _create_time(std::chrono::steady_clock::now()), _last_active(_create_time) {
        try {
            // 初始化MySQL连接
            _mysql = mysql_init(nullptr);
            if (!_mysql) {
                throw std::runtime_error("MySQL初始化失败");
            }

            // 设置连接选项
            _set_options();

            // 建立连接
            if (!mysql_real_connect(
                _mysql,
                _opts.host.c_str(),
                _opts.username.c_str(),
                _opts.password.c_str(),
                _opts.database.c_str(),
                _opts.port,
                _opts.type == ConnectionType::SOCKET ? _opts.socket_path.c_str() : nullptr,
                _opts.client_flag
            )) {
                std::string error = mysql_error(_mysql);
                mysql_close(_mysql);
                _mysql = nullptr;
                throw std::runtime_error("MySQL连接失败: " + error);
            }

            //// 选择数据库
            //_select_db();
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("创建MySQL连接发生异常: {}", e.what()));
            if (_mysql) {
                mysql_close(_mysql);
                _mysql = nullptr;
            }
            throw;
        }
    }

    Connection::Connection(const ConnectionOptions& opts, Dummy)
        : _opts(opts), _create_time(std::chrono::steady_clock::now()), _last_active(_create_time), _mysql(nullptr), _owns_mysql(true) {
        // 这个构造函数不创建实际连接，由连接池负责创建
    }

    Connection::Connection(Connection&& other) noexcept
        : _mysql(nullptr), _owns_mysql(true), _create_time(), _last_active(), _opts() {
        swap(*this, other);
    }

    Connection& Connection::operator=(Connection&& other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    Connection::~Connection() {
        try {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_mysql && _owns_mysql) {
                mysql_close(_mysql);
                _mysql = nullptr;
            }
        }
        catch (...) {
            // 析构函数中不应抛出异常，但应记录错误
            LOGGER_ERROR("Connection析构函数发生异常");
        }
    }

    bool Connection::broken() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return !_mysql || mysql_ping(_mysql) != 0;
    }

    void Connection::reset() noexcept {
        // 在MySQL中，没有直接的方法重置连接状态
        // 我们可以通过执行一个简单的查询来刷新连接
        execute_query("SELECT 1");
    }

    void Connection::invalidate() noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_mysql && _owns_mysql) {
            mysql_close(_mysql);
            _mysql = nullptr;
        }
    }

    void Connection::reconnect() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_mysql && _owns_mysql) {
            mysql_close(_mysql);
            _mysql = nullptr;
        }

        try {
            _mysql = mysql_init(nullptr);
            if (!_mysql) {
                throw std::runtime_error("MySQL初始化失败");
            }

            _set_options();

            if (!mysql_real_connect(
                _mysql,
                _opts.host.c_str(),
                _opts.username.c_str(),
                _opts.password.c_str(),
                _opts.database.c_str(),
                _opts.port,
                _opts.type == ConnectionType::SOCKET ? _opts.socket_path.c_str() : nullptr,
                _opts.client_flag
            )) {
                std::string error = mysql_error(_mysql);
                mysql_close(_mysql);
                _mysql = nullptr;
                throw std::runtime_error("MySQL重新连接失败: " + error);
            }

            _last_active = std::chrono::steady_clock::now();
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("重新连接MySQL发生异常: {}", e.what()));
            if (_mysql) {
                mysql_close(_mysql);
                _mysql = nullptr;
            }
            throw;
        }
    }

    bool Connection::execute_query(const std::string& query) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_mysql) {
            LOGGER_ERROR("MySQL连接无效");
            return false;
        }

        try {
            int state = mysql_query(_mysql, query.c_str());
            if (state != 0) {
                std::string error = mysql_error(_mysql);
                LOGGER_ERROR(std::format("执行查询失败: {}", error));
                return false;
            }

            MYSQL_RES* result = mysql_store_result(_mysql);
            if (result) {
                mysql_free_result(result);
            }

            // 更新最后活动时间
            _last_active = std::chrono::steady_clock::now();
            return true;
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("执行查询时发生异常: {}", e.what()));
            return false;
        }
        catch (...) {
            LOGGER_ERROR("执行查询时发生未知异常");
            return false;
        }
    }

    bool Connection::start_transaction() {
        return execute_query("START TRANSACTION");
    }

    bool Connection::commit() {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_mysql) {
            LOGGER_ERROR("MySQL连接无效");
            return false;
        }

        try {
            if (mysql_commit(_mysql) != 0) {
                std::string error = mysql_error(_mysql);
                LOGGER_ERROR(std::format("提交事务失败: {}", error));
                return false;
            }

            // 更新最后活动时间
            _last_active = std::chrono::steady_clock::now();
            return true;
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("提交事务时发生异常: {}", e.what()));
            return false;
        }
        catch (...) {
            LOGGER_ERROR("提交事务时发生未知异常");
            return false;
        }
    }

    bool Connection::rollback() {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_mysql) {
            LOGGER_ERROR("MySQL连接无效");
            return false;
        }

        try {
            if (mysql_rollback(_mysql) != 0) {
                std::string error = mysql_error(_mysql);
                LOGGER_ERROR(std::format("回滚事务失败: {}", error));
                return false;
            }

            // 更新最后活动时间
            _last_active = std::chrono::steady_clock::now();
            return true;
        }
        catch (const std::exception& e) {
            LOGGER_ERROR(std::format("回滚事务时发生异常: {}", e.what()));
            return false;
        }
        catch (...) {
            LOGGER_ERROR("回滚事务时发生未知异常");
            return false;
        }
    }

    MYSQL* Connection::get_mysql() noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return _mysql;
    }

    bool Connection::is_valid() {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_mysql) {
            return false;
        }

        try {
            // 执行简单查询检查连接是否有效
            int state = mysql_query(_mysql, "SELECT 1");
            if (state != 0) {
                return false;
            }

            MYSQL_RES* result = mysql_store_result(_mysql);
            if (result) {
                mysql_free_result(result);
            }

            // 更新最后活动时间
            _last_active = std::chrono::steady_clock::now();
            return true;
        }
        catch (...) {
            // 任何异常都表示连接无效
            return false;
        }
    }

    /*bool Connection::is_expired(std::chrono::milliseconds timeout) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - _last_active) > timeout;
    }*/

    void Connection::_set_options() {
        if (!_mysql) return;

        unsigned int protocol = static_cast<unsigned int>(_opts.type);
        mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &protocol);

        mysql_options(_mysql, MYSQL_REPORT_DATA_TRUNCATION, &_opts.is_report_data_truncation);

        if (!_opts.shared_memory_base_name.empty()) {
            mysql_options(_mysql, MYSQL_SHARED_MEMORY_BASE_NAME, _opts.shared_memory_base_name.c_str());
        }

        // 设置连接超时
        unsigned int timeout = static_cast<unsigned int>(_opts.connect_timeout.count());
        mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

        // 设置操作超时
        if (_opts.type == ConnectionType::TCP) {
            unsigned int read_timeout = static_cast<unsigned int>(_opts.tcp_read_timeout.count());
            unsigned int write_timeout = static_cast<unsigned int>(_opts.tcp_write_timeout.count());
            mysql_options(_mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
            mysql_options(_mysql, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
        }

        // 设置字符集
        mysql_options(_mysql, MYSQL_SET_CHARSET_NAME, _opts.charset.c_str());

        // 设置自动重连
        if (_opts.keep_alive) {
            char reconnect = 1;
            mysql_options(_mysql, MYSQL_OPT_RECONNECT, &reconnect);
        }

        if (!_opts.read_default_group.empty() && !_opts.read_default_file.empty()) {
            mysql_options(_mysql, MYSQL_READ_DEFAULT_FILE, _opts.read_default_file.c_str());
            mysql_options(_mysql, MYSQL_READ_DEFAULT_GROUP, _opts.read_default_group.c_str());
        }
    }

    void Connection::_select_db() {
        if (!_mysql || _opts.database.empty()) return;

        if (mysql_select_db(_mysql, _opts.database.c_str()) != 0) {
            std::string error = mysql_error(_mysql);
            throw std::runtime_error("选择数据库失败: " + error);
        }
    }

    MYSQL* Connection::_context() {
        std::lock_guard<std::mutex> lock(_mutex);
        _last_active = std::chrono::steady_clock::now();
        return _mysql;
    }
}