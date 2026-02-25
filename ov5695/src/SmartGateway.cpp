#include "SmartGateway.h"
#include "Logger.h"  // 🌟 引入日志系统
#include "MqttManager.h"
#include "CameraManager.h"

SmartGateway::SmartGateway() {
    mg_log_set(0); // 禁用 Mongoose 自带的简陋日志，改用我们自己的
    mg_mgr_init(&mgr);
}

SmartGateway::~SmartGateway() {
    mg_mgr_free(&mgr);
}

void SmartGateway::start(const std::string& port) {
    std::string addr = "0.0.0.0:" + port;
    
    if (mg_http_listen(&mgr, addr.c_str(), ev_handler, nullptr) == nullptr) {
        LOG_E("Web", "致命错误: 无法监听端口 %s", port.c_str());
        return;
    }

    is_running = true;
    LOG_I("Web", "🚀 RK3568 智能网关 Web 服务已启动: %s", addr.c_str());

    while (is_running) {
        mg_mgr_poll(&mgr, 100); // 缩短轮询间隔，响应更及时
    }
}

void SmartGateway::ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        auto *hm = static_cast<struct mg_http_message *>(ev_data);
        auto& gateway = SmartGateway::getInstance();

        // 记录每一个进入的请求 (DEBUG 级别，防止正式运行日志太杂)
        LOG_D("Web", "收到请求: %.*s", (int)hm->uri.len, hm->uri.buf);

        if (mg_match(hm->uri, mg_str("/api/status"), NULL)) {
            gateway.handle_status(c);
        } 
        else if (mg_match(hm->uri, mg_str("/api/control"), NULL)) {
            gateway.handle_control(c, hm);
        } 
        else if (mg_match(hm->uri, mg_str("/api/camera/snapshot"), NULL)) {
            gateway.handle_snapshot(c);
        } 
        else if (mg_match(hm->uri, mg_str("/api/camera/start_record"), NULL)) {
            gateway.handle_start_record(c);
        } 
        else if (mg_match(hm->uri, mg_str("/api/camera/stop_record"), NULL)) {
            gateway.handle_stop_record(c);
        } 
        else {
            // 静态文件托管
            struct mg_http_serve_opts opts = { .root_dir = "/woniu/web" };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

void SmartGateway::handle_status(struct mg_connection *c) {
    SensorData s;
    DeviceStatus d;
    this->getAllStatus(s, d);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temp", s.temp);
    cJSON_AddNumberToObject(root, "humi", s.humi);
    cJSON_AddNumberToObject(root, "light", s.light);
    cJSON_AddNumberToObject(root, "ir", s.ir_val);
    
    cJSON_AddNumberToObject(root, "led_on", d.led_on);
    cJSON_AddNumberToObject(root, "led_br", d.led_br);
    cJSON_AddNumberToObject(root, "motor_on", d.motor_on);
    cJSON_AddNumberToObject(root, "motor_sp", d.motor_sp);
    cJSON_AddNumberToObject(root, "motor_dir", d.motor_dir);
    cJSON_AddNumberToObject(root, "buzzer", d.buzzer);

    char *json = cJSON_PrintUnformatted(root);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
    
    free(json);
    cJSON_Delete(root);
}

void SmartGateway::handle_control(struct mg_connection *c, struct mg_http_message *hm) {
    char buf[1024] = {0};
    if(hm->body.len >= sizeof(buf)-1) {
         LOG_W("Web", "控制请求体过大");
         mg_http_reply(c, 500, "", "Body too large");
         return;
    }
    memcpy(buf, hm->body.buf, hm->body.len);
    cJSON *root = cJSON_Parse(buf);

    if (root) {
        char *payload = cJSON_PrintUnformatted(root);
        LOG_I("Web", "收到 Web 控制指令，转发至 MQTT: %s", payload);
        
        MqttManager::getInstance().publish("woniu/control", payload);
        
        free(payload);
        cJSON_Delete(root);
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\":\"ok\"}");
    } else {
        LOG_E("Web", "解析 Web 指令 JSON 失败: %s", buf);
        mg_http_reply(c, 400, "", "JSON Parse Error");
    }
}

void SmartGateway::handle_snapshot(struct mg_connection *c) {
    char filename[64];
    time_t now = time(0);
    strftime(filename, sizeof(filename), "photo_%Y%m%d_%H%M%S.jpg", localtime(&now));
    
    // 确保目录存在，防止路径报错
    system("mkdir -p /woniu/photos");
    std::string fullPath = "/woniu/photos/" + std::string(filename);

    LOG_I("Web", "📸 收到网页抓拍请求，准备保存至: %s", fullPath.c_str());
    
    // 🌟 核心修改：直接调用 CameraManager 封装好的接口
    // (注意：这里的 camera 替换成你实际在 SmartGateway 中定义的 CameraManager 实例名)
    if (CameraManager::getInstance().takeSnapshot(fullPath)) {
        // 抓拍成功，返回 JSON 和文件名给前端
        std::string resp = "{\"status\":\"success\", \"file\":\"" + std::string(filename) + "\"}";
        // 注意：mg_http_reply 最好加上 "%s" 防止字符串中出现意外的格式化字符
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", resp.c_str());
    } else {
        // 抓拍失败，返回 500 错误码
        LOG_E("Web", "抓拍执行失败，请检查底层流媒体是否正常");
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"status\":\"failed\"}");
    }
}

void SmartGateway::handle_start_record(struct mg_connection *c) {
    time_t now = time(NULL);
    char filename[64];
    strftime(filename, sizeof(filename), "VID_%Y%m%d_%H%M%S.mp4", localtime(&now));
    system("mkdir -p /woniu/videos");
    std::string path = "/woniu/videos/" + std::string(filename);

    LOG_I("Web", "🔴 收到录像请求，目标文件: %s", path.c_str());

    if (CameraManager::getInstance().startRecord(path)) {
        LOG_I("Web", "录像进程已成功拉起");
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\":\"ok\"}");
    } else {
        LOG_E("Web", "录像进程启动失败");
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"result\":\"error\"}");
    }
}

void SmartGateway::handle_stop_record(struct mg_connection *c) {
    LOG_I("Web", "⏹ 收到停止录像请求");
    CameraManager::getInstance().stopRecord();
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\":\"ok\"}");
}