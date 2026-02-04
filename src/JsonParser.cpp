#include "JsonParser.hpp"

Stm32SensorData JsonParser::parseStm32Data(const std::string& jsonStr) {
    Stm32SensorData result;
    cJSON *root = cJSON_Parse(jsonStr.c_str());
    
    if (root == nullptr) {
        std::cerr << "[JSON] 格式错误，无法解析" << std::endl;
        return result;
    }

    try {
        // 1. 解析外层数据
        cJSON *source = cJSON_GetObjectItem(root, "source");
        cJSON *driver = cJSON_GetObjectItem(root, "driver");
        cJSON *id = cJSON_GetObjectItem(root, "id");

        if (source) result.source = source->valuestring;
        if (driver) result.driver = driver->valuestring;
        if (id)     result.id = id->valueint;

        // 2. 解析嵌套的 data 对象
        cJSON *data_obj = cJSON_GetObjectItem(root, "data");
        if (data_obj && cJSON_IsObject(data_obj)) {
            // 提取温湿度各部分
            int h_int  = cJSON_GetObjectItem(data_obj, "humi_int")->valueint;
            int h_deci = cJSON_GetObjectItem(data_obj, "humi_deci")->valueint;
            int t_int  = cJSON_GetObjectItem(data_obj, "temp_int")->valueint;
            int t_deci = cJSON_GetObjectItem(data_obj, "temp_deci")->valueint;
            
            // 提取时间戳和校验和
            result.data_time = (long long)cJSON_GetObjectItem(data_obj, "data_time")->valuedouble;
            result.check_sum = cJSON_GetObjectItem(data_obj, "check_sum")->valueint;

            // 3. 计算最终浮点值
            result.temperature = t_int + (t_deci / 10.0f);
            result.humidity = h_int + (h_deci / 10.0f);
            
            result.is_valid = true;
        }
    } catch (...) {
        std::cerr << "[JSON] 字段解析异常" << std::endl;
    }

    // 4. 释放内存是嵌入式开发的重中之重
    cJSON_Delete(root);
    return result;
}
