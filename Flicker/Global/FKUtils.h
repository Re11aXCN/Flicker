/*************************************************************************************
 *
 * @ Filename	 : FKUtils.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_UTILS_H_
#define FK_UTILS_H_
#include <cctype> 
#include <array>
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <utility>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#ifdef QT_CORE_LIB
#include <QString>
#include <QColor>
#endif
namespace FKUtils {
	namespace helper {
		// 辅助函数：将半字节(0-15)转换为16进制字符（大写）
		constexpr unsigned char nibble_to_hex(unsigned char nibble) noexcept {
			nibble &= 0x0F; // 确保只有低4位
			return nibble < 10 ? ('0' + nibble) : ('A' + nibble - 10);
		}

		// 辅助函数：将16进制字符转换为半字节(0-15)
		constexpr unsigned char hex_to_nibble(unsigned char c) {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			throw std::invalid_argument("Invalid hexadecimal character");
		}
	};

	/**
	 * @brief : 将两个16进制字符解码为一个十进制字节
	 * @usage :  unsigned char byte = hex_decode('1', 'A'); // 0x1A -> 26
	 */
	template <typename T = unsigned char>
		requires std::same_as<T, unsigned char>
	constexpr T hex_decode(unsigned char high, unsigned char low) {
		return (helper::hex_to_nibble(high) << 4) | helper::hex_to_nibble(low);
	}

	/**
	 * @brief : 将一个十进制字节编码为两个16进制字符（返回数组）
	 * @usage : auto [high, low] = hex_encode(26u); // 返回 {'1', 'A'}
	 */
	template <typename T>
		requires std::same_as<T, unsigned char>
	constexpr std::array<unsigned char, 2> hex_encode(T byte) noexcept {
		return {
			helper::nibble_to_hex(static_cast<unsigned char>(byte) >> 4), // 高4位
			helper::nibble_to_hex(byte)                                  // 低4位
		};
	}

	/**
	 * @brief : 批量解码：将16进制数组转换为十进制字节数组
	 * @usage : 
		std::vector<unsigned char> hex_data = {'A','1','F','F'};
		std::vector<unsigned char> decoded;
		hex_decode_bulk(hex_data.begin(), hex_data.end(), std::back_inserter(decoded));
		// decoded = [161, 255]
	 */
	template <typename InputIt, typename OutputIt>
	constexpr void hex_decode_bulk(InputIt first, InputIt last, OutputIt out) {
		if (std::distance(first, last) % 2 != 0)
			throw std::invalid_argument("Hex data length must be even");

		while (first != last) {
			unsigned char high = *first++;
			unsigned char low = *first++;
			*out++ = hex_decode(high, low);
		}
	}

	/**
	 * @brief : 批量编码：将十进制字节数组转换为16进制数组
	 * @usage :
		std::vector<unsigned char> bytes = {10, 255};
		std::vector<unsigned char> encoded;
		hex_encode_bulk(bytes.begin(), bytes.end(), std::back_inserter(encoded));
		// encoded = {'0','A','F','F'}
	 */
	template <typename InputIt, typename OutputIt>
	constexpr void hex_encode_bulk(InputIt first, InputIt last, OutputIt out) {
		while (first != last) {
			auto hex_pair = hex_encode(*first++);
			*out++ = hex_pair[0];
			*out++ = hex_pair[1];
		}
	}

	/**
	 * @brief URL 编码（类似 Qt 的 toPercentEncoding）
	 * @param input 要编码的字符串
	 * @return URL 编码后的字符串
	 *
	 * 编码规则：
	 * 1. 字母、数字、连字符(-)、下划线(_)、点(.)、波浪线(~) 不编码
	 * 2. 空格编码为 '+' 或 "%20"（默认使用 %20）
	 * 3. 其他字符编码为 %HH 格式（HH 是大写十六进制值）
	 */
	inline std::string url_encode(std::string_view input, bool spaceAsPlus = false) {
		std::string result;
		// 预分配空间（最坏情况下每个字符变为3个字符）
		result.reserve(input.size() * 3);

		// 安全字符集（直接保留不编码）
		static const std::unordered_set<char> safe_chars = {
			'-', '_', '.', '~'
		};

		for (unsigned char c : input) {
			if (std::isalnum(c) || safe_chars.count(c)) {
				// 安全字符直接保留
				result.push_back(static_cast<char>(c));
			}
			else if (c == ' ' && spaceAsPlus) {
				// 空格特殊处理（查询参数中常用+）
				result.push_back('+');
			}
			else {
				// 其他字符转换为 %HH 格式
				auto hex = hex_encode(c);
				result.push_back('%');
				result.push_back(static_cast<char>(hex[0]));
				result.push_back(static_cast<char>(hex[1]));
			}
		}

		return result;
	}

	/**
	 * @brief URL 解码（类似 Qt 的 fromPercentEncoding）
	 * @param input 要解码的字符串
	 * @return 解码后的原始字符串
	 *
	 * 解码规则：
	 * 1. %HH 格式转换为对应字节
	 * 2. '+' 转换为空格
	 * 3. 其他字符保留不变
	 */
	inline std::string url_decode(std::string_view input) {
		std::string result;
		result.reserve(input.size()); // 解码后长度不会超过输入

		for (size_t i = 0; i < input.size(); ++i) {
			char c = input[i];

			if (c == '+') {
				// 加号解码为空格
				result.push_back(' ');
			}
			else if (c == '%' && i + 2 < input.size()) {
				// 处理 %HH 格式
				try {
					unsigned char byte = hex_decode(
						static_cast<unsigned char>(input[i + 1]),
						static_cast<unsigned char>(input[i + 2])
					);
					result.push_back(static_cast<char>(byte));
					i += 2; // 跳过已处理的两位十六进制
				}
				catch (const std::invalid_argument&) {
					// 无效的十六进制字符，保留 % 原样输出
					result.push_back('%');
				}
			}
			else {
				// 其他字符直接保留
				result.push_back(c);
			}
		}

		return result;
	}

	/**
	 * @brief 高效解析 URI 查询参数
	 * @param uri 
	 * @param url 如果uri包含"?"，则url为uri"?"的前部分，url = uri
	 * @return 解析后的查询参数键值对
	 */
	inline std::unordered_map<std::string, std::string> parse_query_params(std::string_view uri, std::string& url) {
		std::unordered_map<std::string, std::string> params;

		// 1. 定位查询字符串起始位置
		const size_t query_start = uri.find('?');
		if (query_start == std::string_view::npos) {
			url = uri;		// 无查询参数，返回原始 URI
			return params; // 没有查询参数
		}

		// 2. 提取查询字符串部分（忽略片段标识符）
		url = uri.substr(0, query_start);
		std::string_view query_str = uri.substr(query_start + 1);
		if (const size_t fragment_pos = query_str.find('#');
			fragment_pos != std::string_view::npos) {
			query_str = query_str.substr(0, fragment_pos);
		}

		// 3. 预分配内存（基于&字符数量）
		const size_t approx_param_count =
			std::count(query_str.begin(), query_str.end(), '&') + 1;
		params.reserve(approx_param_count);

		// 4. 高效状态机解析
		while (!query_str.empty()) {
			// 找到下一个键值对边界
			const size_t pair_end = query_str.find('&');
			std::string_view pair = query_str.substr(0, pair_end);

			// 分割键和值
			const size_t eq_pos = pair.find('=');
			std::string_view key = pair.substr(0, eq_pos);
			std::string_view value = (eq_pos == std::string_view::npos)
				? std::string_view{}
			: pair.substr(eq_pos + 1);

			// 解码并存储（移动语义避免拷贝）
			params.try_emplace(
				url_decode(key),
				url_decode(value)
			);

			// 移动到下一对参数
			query_str = (pair_end == std::string_view::npos)
				? std::string_view{}
			: query_str.substr(pair_end + 1);
		}

		return params;
	}

	// 静态方法：将时间点转换为格式化字符串
	inline std::string format_time_point(const std::chrono::system_clock::time_point& timePoint) {
		auto time = std::chrono::system_clock::to_time_t(timePoint);
		std::stringstream ss;
		// 使用线程安全的方式格式化时间
		std::tm tmBuf{};
#ifdef _WIN32
		localtime_s(&tmBuf, &time);
#else
		localtime_r(&time, &tmBuf);
#endif
		ss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	// 获取HTTP日期格式的当前时间
	inline std::string get_http_date() {
		// 获取当前时间点
		auto now = std::chrono::system_clock::now();
		auto time_t_now = std::chrono::system_clock::to_time_t(now);

		// 转换为GMT时间
		std::tm gmt_tm;
#ifdef _WIN32
		gmtime_s(&gmt_tm, &time_t_now);
#else
		gmtime_r(&time_t_now, &gmt_tm);
#endif

		// 格式化为HTTP日期格式 (RFC 7231)
		// 例如: "Sun, 06 Nov 1994 08:49:37 GMT"
		std::stringstream ss;
		ss << std::put_time(&gmt_tm, "%a, %d %b %Y %H:%M:%S GMT");
		return ss.str();
	}
};
#ifdef QT_CORE_LIB
namespace FKUtils {
	// 颜色字符串格式枚举
	enum class ColorStringFormat {
		HexARGB,    // #AARRGGBB格式
		RGBAFunction // rgba(r,g,b,a)格式
	};

	namespace helper {
		inline QString toQString(const QString& str) { return str; }
		inline QString toQString(const char* str) { return QString(str); }
		inline QString toQString(const std::string& str) { return QString::fromStdString(str); }
		inline QString toQString(const QLatin1String& str) { return QString(str); }
		
		template<typename T>
		inline auto toQString(const T& value) ->
			typename std::enable_if_t<std::is_arithmetic_v<T>, QString> {
			return QString::number(value);
		}

		inline QString toHexString(uint value, int width = 2) {
			// 直接构造16进制字符串避免格式化开销
			constexpr const char* hexDigits = "0123456789ABCDEF";
			QString result(width, Qt::Uninitialized);
			QChar* data = result.data();

			for (int i = width - 1; i >= 0; --i) {
				data[i] = QLatin1Char(hexDigits[value & 0xF]);
				value >>= 4;
			}
			return result;
		}
	};

	template<typename... Args>
	inline QString concat(Args&&... args) {
		int totalSize = (0 + ... + helper::toQString(args).size());
		QString result;
		result.reserve(totalSize);
		(result += ... += helper::toQString(std::forward<Args>(args)));
		return result;
	}

	template<typename... Args>
	inline QString join(const QString& separator, Args&&... args) {
		if constexpr (sizeof...(args) == 0) return QString();
		int separatorCount = sizeof...(args) - 1;
		int totalSize = (0 + ... + toQString(args).size()) + separatorCount * separator.size();
		QString result;
		result.reserve(totalSize);
		int index = 0;
		((result += (index++ > 0 ? separator : QString()) +
			toQString(std::forward<Args>(args))), ...);
		return result;
	}

	// 主转换函数模板
	template<ColorStringFormat Format = ColorStringFormat::HexARGB>
	QString colorToCssString(const QColor& color) {
		if (!color.isValid()) return QLatin1String("invalid");

		using namespace helper;

		if constexpr (Format == ColorStringFormat::HexARGB) {
			// 使用concat高效拼接
			return concat(QLatin1String("#"),
				toHexString(color.alpha()),
				toHexString(color.red()),
				toHexString(color.green()),
				toHexString(color.blue()));
		}
		else {
			// 使用join构建RGBA函数
			return join(QLatin1String(", "),
				concat(QLatin1String("rgba("), color.red()),
				color.green(),
				color.blue(),
				concat(QString::number(color.alphaF(), 'f', 3),
					QLatin1String(")")));
		}
	}
};
#endif
#endif // !FK_UTILS_H_
