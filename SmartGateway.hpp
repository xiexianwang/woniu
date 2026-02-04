#ifndef SMART_GATEWAY_HPP
#define SMART_GATEWAY_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "mongoose.h"
#include "cJSON.h"

// 采用单例模式确保网关资源的唯一性
class SmartGateway {
public:
    // C++14 线程安全的单例获取接口
    static SmartGateway& getInstance() {
        static SmartGateway instance;
        return instance;
    }

    // 禁用拷贝与赋值操作
    SmartGateway(const SmartGateway&) = delete;
    SmartGateway& operator=(const SmartGateway&) = delete;

    void start(const std::string& port = "80");
    void stop() { is_running = false; }

private:
    SmartGateway();
    ~SmartGateway();

    // Mongoose 回调逻辑
    static void ev_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
    
    // API 路由处理
    void handle_status(struct mg_connection *c);
    void handle_control(struct mg_connection *c, struct mg_http_message *hm);
    void handle_snapshot(struct mg_connection *c);

    // 硬件抽象层：读取 RK3568 系统温度
    float read_cpu_temperature();

    struct mg_mgr mgr{};
    bool is_running{false};
};

#endif
