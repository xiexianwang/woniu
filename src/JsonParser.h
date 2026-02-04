#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <string>
#include <iostream>
#include "cJSON.h"

// 定义结构体，用于存放解析后的 STM32 数据
struct Stm32SensorData {
    std::string source;
    std::string driver;
    int id;
    long long data_time;
    float temperature;
    float humidity;
    int check_sum;
    bool is_valid = false; // 标记解析是否成功
};

class JsonParser {
public:
    // 静态工具函数，输入 JSON 字符串，返回解析后的结构体
    static Stm32SensorData parseStm32Data(const std::string& jsonStr);
};

#endif
