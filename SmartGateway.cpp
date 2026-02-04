#include "SmartGateway.hpp"

SmartGateway::SmartGateway() {
    mg_mgr_init(&mgr);
}

SmartGateway::~SmartGateway() {
    mg_mgr_free(&mgr);
}

void SmartGateway::start(const std::string& port) {
    std::string addr = "http://0.0.0.0:" + port;
    if (mg_http_listen(&mgr, addr.c_str(), ev_handler, nullptr) == nullptr) {
        std::cerr << "错误: 无法监听端口 " << port << std::endl;
        return;
    }

    is_running = true;
    std::cout << "[woniu] RK3568 智能网关已启动: " << addr << std::endl;

    while (is_running) {
        mg_mgr_poll(&mgr, 1000);
    }
}

void SmartGateway::ev_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        auto *hm = static_cast<struct mg_http_message *>(ev_data);
        auto& gateway = SmartGateway::getInstance();

        // 路径分发
        if (mg_http_match_uri(hm, "/api/status")) {
            gateway.handle_status(c);
        } else if (mg_http_match_uri(hm, "/api/control")) {
            gateway.handle_control(c, hm);
        } else if (mg_http_match_uri(hm, "/api/camera/snapshot")) {
            gateway.handle_snapshot(c);
        } else {
            // 托管静态网页文件
            struct mg_http_serve_opts opts = { .root_dir = "/usr/www" };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

float SmartGateway::read_cpu_temperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return 0.0f;
    int raw_temp;
    file >> raw_temp;
    return static_cast<float>(raw_temp) / 1000.0f;
}

void SmartGateway::handle_status(struct mg_connection *c) {
    cJSON *root = cJSON_CreateObject();
    // 对接网页 val_temp, val_humi 等字段
    cJSON_AddNumberToObject(root, "temp", read_cpu_temperature());
    cJSON_AddNumberToObject(root, "humi", 45); 
    cJSON_AddNumberToObject(root, "light", 300);
    cJSON_AddNumberToObject(root, "ir", 1024); // 模拟传感器状态

    char *json = cJSON_PrintUnformatted(root);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
    
    free(json);
    cJSON_Delete(root);
}

void SmartGateway::handle_control(struct mg_connection *c, struct mg_http_message *hm) {
    cJSON *root = cJSON_ParseWithLength(hm->body.ptr, hm->body.len);
    if (root) {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload) {
            int led = cJSON_GetObjectItem(payload, "led_on")->valueint;
            std::cout << "[控制] LED 状态同步为: " << (led ? "ON" : "OFF") << std::endl;
            // 以后在这里添加 GPIO 操作逻辑
        }
        mg_http_reply(c, 200, "", "{\"status\":\"success\"}");
        cJSON_Delete(root);
    }
}

void SmartGateway::handle_snapshot(struct mg_connection *c) {
    // 跨进程通信：请求 8080 端口的 mjpg-streamer 抓取快照
    std::string cmd = "wget http://127.0.0.1:8080/?action=snapshot -O /usr/www/snapshot.jpg";
    int ret = system(cmd.c_str());
    
    if (ret == 0) {
        std::cout << "[视频] 抓拍成功并保存至 /usr/www/snapshot.jpg" << std::endl;
        mg_http_reply(c, 200, "", "{\"status\":\"success\"}");
    } else {
        mg_http_reply(c, 500, "", "{\"status\":\"failed\"}");
    }
}
