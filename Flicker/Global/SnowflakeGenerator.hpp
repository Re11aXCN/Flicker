#ifndef SNOWFLAKEGENERATOR_HPP
#define SNOWFLAKEGENERATOR_HPP

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

// 雪花算法模板类
template <
    typename T = uint64_t,
    std::size_t TimestampBits = 41, // 时间戳位数（默认41位，约69年）
    std::size_t MachineIdBits = 10, // 机器ID位数（默认10位，1024台机器）
    std::size_t SequenceBits = 12,  // 序列号位数（默认12位，每毫秒4096个ID）
    uint64_t Epoch = 1609459200000ULL // 起始时间戳 2021-01-01 00:00:00 UTC
>
    requires std::unsigned_integral<T> &&
(TimestampBits + MachineIdBits + SequenceBits + 1 == sizeof(T) * 8)
class SnowflakeGenerator {
public:
    // 静态断言验证参数有效性
    static_assert(TimestampBits > 0 && MachineIdBits > 0 && SequenceBits > 0,
        "Bits must be positive");
    static_assert(sizeof(T) * 8 == TimestampBits + MachineIdBits + SequenceBits + 1,
        "Total bits must match type size");
    static_assert(TimestampBits <= 42, "Timestamp too large for millisecond precision");
    static_assert(SequenceBits > 1, "Sequence bits too small");

    // 计算各段最大值
    static constexpr T kMaxMachineId = (T{ 1 } << MachineIdBits) - 1;
    static constexpr T kMaxSequence = (T{ 1 } << SequenceBits) - 1;
    static constexpr T kMaxTimestamp = (T{ 1 } << TimestampBits) - 1;

    // 构造函数
    explicit SnowflakeGenerator(T machine_id)
        : machine_id_(machine_id),
        last_timestamp_(0),
        sequence_(0) {
        if (machine_id > kMaxMachineId) {
            throw std::invalid_argument("Machine ID exceeds maximum value");
        }
    }

    // 生成唯一ID
    T generate() {
        T current_timestamp = current_time();
        T old_timestamp = last_timestamp_.load(std::memory_order_relaxed);

        // 处理时间回退
        if (current_timestamp < old_timestamp) {
            handle_clock_backwards(old_timestamp, current_timestamp);
        }

        // 同一毫秒内的序列处理
        if (current_timestamp == old_timestamp) {
            T seq = sequence_.fetch_add(1, std::memory_order_relaxed) & kMaxSequence;

            // 序列号溢出，等待下一毫秒
            if (seq == 0) {
                current_timestamp = wait_next_millis(old_timestamp);
            }
        }
        else {
            sequence_.store(0, std::memory_order_relaxed);
        }

        // 更新最后时间戳
        last_timestamp_.store(current_timestamp, std::memory_order_relaxed);

        // 组合生成ID
        return ((current_timestamp << (MachineIdBits + SequenceBits)) |
            (machine_id_ << SequenceBits) |
            (sequence_.fetch_add(1, std::memory_order_relaxed) & kMaxSequence);
    }

private:
    // 获取当前时间戳
    T current_time() const {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch());
        return static_cast<T>(ms.count() - Epoch);
    }

    // 处理时钟回退
    void handle_clock_backwards(T old_timestamp, T current_timestamp) {
        auto diff = old_timestamp - current_timestamp;
        if (diff > max_clock_backward_ms) {
            throw std::runtime_error("Clock moved backwards more than allowed");
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(diff + 1));
    }

    // 等待下一毫秒
    T wait_next_millis(T last_timestamp) const {
        T timestamp = current_time();
        while (timestamp <= last_timestamp) {
            std::this_thread::yield();
            timestamp = current_time();
        }
        return timestamp;
    }

    // 成员变量
    const T machine_id_;
    std::atomic<T> last_timestamp_;
    std::atomic<T> sequence_;

    // 允许的最大时钟回退（毫秒）
    static constexpr T max_clock_backward_ms = 100;
};

// 别名模板简化使用
using StandardSnowflake = SnowflakeGenerator<>;
/*
// 示例用法
int main() {
    // 创建雪花生成器（机器ID为123）
    StandardSnowflake generator(123);

    // 生成10个ID
    for (int i = 0; i < 10; ++i) {
        auto id = generator.generate();
        // 在实际应用中使用生成的ID
        // std::cout << "Generated ID: " << id << "\n";
    }
    using CustomSnowflake = SnowflakeGenerator<
        uint64_t,   // ID类型
        39,         // 时间戳位数（约17年）
        12,         // 机器ID位数（4096台）
        12,         // 序列号位数
        1672531200000ULL // 2023-01-01起始时间
    >;
    CustomSnowflake generator(2049); // 机器ID

    // 生成分布式ID
    auto order_id = generator.generate();
    auto user_id = generator.generate();
    return 0;
}
*/
#endif // !SNOWFLAKEGENERATOR_HPP
