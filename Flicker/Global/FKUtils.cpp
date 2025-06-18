#include "FKUtils.h"
#include <print>
#include <iostream>
void Test_URL_D_Encode() {
	// 原始字符串（包含中文和特殊字符）
	std::string text = "http://www.baidu.com/s?ie=utf-8&f=8&tn=baidu&wd=临时邮箱";

	// URL 编码
	std::string encoded = FKUtils::url_encode(text);
	std::println("编码后:\n{}\n\n", encoded);

	// URL 解码
	std::string decoded = FKUtils::url_decode(encoded);
	std::cout << "-------------------------------------------------\n";
	std::println("解码后:\n{}\n\n", decoded);

}

void Test_Http_Get_Query_Params() {
	// 带查询参数的 URL
	std::string_view uri =
		"https://www.example.com/search"
		"?q=C%2B%2B+Programming"
		"&lang=zh-CN"
		"&price=29.99"
		"&discount=50%25"
		"&filter=new"
		"&filter=popular"
		"&empty_value"
		"&special_chars=%21%40%23%24%25%5E%26*";

	// 解析查询参数
	std::string url;
	auto params = FKUtils::parse_query_params(uri, url);
	std::println("url is : {}\n", url) ;
	// 打印解析结果
	std::cout << "解析结果:\n";
	for (const auto& [key, value] : params) {
		std::cout << "  " << key << " = " << value << "\n";
	}

}