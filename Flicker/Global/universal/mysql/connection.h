/*************************************************************************************
 *
 * @ Filename     : connection.h
 * @ Description : MySQL连接封装类，使用纯C API，参考redis++设计
 * 
 * @ Version     : V3.0
 * @ Author      : Re11a
 * @ Date Created: 2025/6/22
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef UNIVERSAL_MYSQL_CONNECTION_H_
#define UNIVERSAL_MYSQL_CONNECTION_H_

#include <string>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <mysql.h>

//https://www.mysqlzh.com/api/48.html
//https://dev.mysql.com/doc/refman/9.3/en/server-system-variables.html#sysvar_named_pipe_full_access_group
namespace universal::mysql {
    // MySQL连接类型
    enum class ConnectionType {
        TCP,
        SOCKET, // Unix Socket连接
        PIPE,   // Windows  named_pipe_full_access_group
        MEMORY  // Windows  Shared-memory 
    };

    // MySQL连接选项
    struct ConnectionOptions {
        /*ConnectionOptions() = default;

        ConnectionOptions(const ConnectionOptions&) = default;
        ConnectionOptions& operator=(const ConnectionOptions&) = default;

        ConnectionOptions(ConnectionOptions&&) = default;
        ConnectionOptions& operator=(ConnectionOptions&&) = default;

        ~ConnectionOptions() = default;*/

        // 连接类型
        ConnectionType type = ConnectionType::TCP;

        // 主机地址
        std::string host{ "127.0.0.1" };

        // Unix Socket路径
        std::string socket_path{};

        // 用户名
        std::string username{};

        // 密码
        std::string password{};

        // 数据库名
        std::string database{};

        // 字符集
        std::string charset{ "utf8mb4" };

        // 配置文件路径
        std::filesystem::path read_default_file{};

        // 配置文件分组
        std::string read_default_group{};

        // 命名为与服务器进行通信的共享内存对象。应与你打算连接的mysqld服务器使用的选项“-shared-memory-base-name”相同。
        std::string shared_memory_base_name{};

        // 连接超时时间
        std::chrono::seconds connect_timeout{ 5 };

        // 操作超时时间
        std::chrono::seconds tcp_read_timeout{ 10 };
        std::chrono::seconds tcp_write_timeout{ 10 };

        // 客户端标志
        unsigned long client_flag{ 0 };

        // 重试次数
        uint32_t retry_count{ 1 };

        // 对于预处理语句，允许或禁止通报数据截断错误（默认为禁止）
        char is_report_data_truncation{ 0 };

        // 是否保持连接
        bool keep_alive{ true };

        // 端口号
        uint16_t port{ 3306 };

        // 获取服务器信息字符串，用于日志和调试
        std::string server_info() const {
            return host + ":" + std::to_string(port) + "/" + database;
        }
    };

    // MySQL连接类
    class Connection {
    public:
        // 构造函数，使用连接选项创建连接
        explicit Connection(const ConnectionOptions& opts);
        
        // 禁止拷贝
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        
        // 允许移动
        Connection(Connection&&) noexcept;
        Connection& operator=(Connection&&) noexcept;
        
        // 析构函数
        ~Connection();

        // 检查连接是否已损坏
        bool broken() const noexcept;
        
        // 重置连接状态
        void reset() noexcept;
        
        // 标记连接为无效
        void invalidate() noexcept;
        
        // 重新连接
        bool reconnect();

        // 获取创建时间
        auto create_time() const
            -> std::chrono::time_point<std::chrono::steady_clock> {
            return _create_time;
        }

        // 获取最后活动时间
        auto last_active() const
            -> std::chrono::time_point<std::chrono::steady_clock> {
            return _last_active;
        }

        // 更新最后活动时间
        void update_active_time() {
            std::lock_guard<std::mutex> lock(_mutex);
            _last_active = std::chrono::steady_clock::now();
        }

        // 执行SQL查询
        bool execute_query(const std::string& query);
        
        // 开始事务
        bool start_transaction();
        
        // 提交事务
        bool commit();
        
        // 回滚事务
        bool rollback();

        // 获取原始MYSQL指针
        MYSQL* get_mysql() noexcept;

        // 获取连接选项
        const ConnectionOptions& options() const {
            return _opts;
        }

        // 检查连接是否有效
        bool is_valid();

        // 检查连接是否过期
        //bool is_expired(std::chrono::milliseconds timeout) const;

        // 交换两个连接
        friend void swap(Connection& lhs, Connection& rhs) noexcept;

    private:
        
        // 设置连接选项
        void _set_options();
        
        // 选择数据库
        void _select_db();

        // 获取MYSQL上下文
        MYSQL* _context();

        // MYSQL指针
        MYSQL* _mysql = nullptr;
        
        // 连接创建时间
        std::chrono::time_point<std::chrono::steady_clock> _create_time;
        
        // 最后活动时间
        std::chrono::time_point<std::chrono::steady_clock> _last_active;
        
        // 连接选项
        ConnectionOptions _opts;
        
        // 互斥锁
        mutable std::mutex _mutex;

        // 声明友元类
        friend class ConnectionPool;
    };

    // 交换两个连接
    inline void swap(Connection& lhs, Connection& rhs) noexcept {
        std::lock_guard<std::mutex> lock_lhs(lhs._mutex);
        std::lock_guard<std::mutex> lock_rhs(rhs._mutex);
        
        std::swap(lhs._mysql, rhs._mysql);
        std::swap(lhs._create_time, rhs._create_time);
        std::swap(lhs._last_active, rhs._last_active);
        std::swap(lhs._opts, rhs._opts);
    }
}

#endif // !UNIVERSAL_MYSQL_CONNECTION_H_