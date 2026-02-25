#include "MqttManager.h"
#include <cJSON.h>
#include "SmartGateway.h" 
#include "Logger.h"  // 🌟 必须引入日志头文件
#include <cmath>

MqttManager::MqttManager() {
    mosquitto_lib_init();
}

MqttManager::~MqttManager() {
    if (mosq) mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

bool MqttManager::init(const std::string& id, const std::string& host, int port) {
    LOG_I("MQTT", "正在启动 MQTT 模块...");

    // 1. 创建实例
    mosq = mosquitto_new(id.c_str(), true, this);
    if (!mosq) return false;

    // 2. 🌟 降级为 V311 增加兼容性测试 🌟
    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V311);

    // 3. 设置回调
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    // 4. 🌟 先启动线程 🌟
    int rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) return false;

    // 5. 🌟 给线程一点启动时间 (100ms) 🌟
    // 让底层的 pthread 真正运行起来
    usleep(100000); 

    // 6. 建立网络连接
    LOG_I("MQTT", "连接目标: %s:%d", host.c_str(), port);
    rc = mosquitto_connect(mosq, host.c_str(), port, 60);
    
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_E("MQTT", "连接请求发送失败: %s", mosquitto_strerror(rc));
        return false;
    }

    // 🌟 这里不要打印“接入成功”，因为还没握手呢！
    LOG_I("MQTT", "连接指令已发送，等待服务器 ACK...");
    return true;
}

bool MqttManager::publish(const std::string& topic, const std::string& message) {
    int rc = mosquitto_publish(mosq, nullptr, topic.c_str(), message.length(), message.c_str(), 1, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_W("MQTT", "发布失败: %s", mosquitto_strerror(rc));
    }
    return (rc == MOSQ_ERR_SUCCESS);
}

bool MqttManager::subscribe(const std::string& topic) {
    int rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 1);
    if (rc == MOSQ_ERR_SUCCESS) {
        LOG_D("MQTT", "订阅成功: %s", topic.c_str());
    } else {
        LOG_E("MQTT", "订阅请求失败: %s", topic.c_str());
    }
    return (rc == MOSQ_ERR_SUCCESS);
}

void MqttManager::on_connect(struct mosquitto *m, void *obj, int rc) {
    if (rc == 0) {
        LOG_I("MQTT", "✅ 成功连接至服务器！");
        
        // 自动订阅控制主题
        MqttManager::getInstance().subscribe("woniu/sensor");
        MqttManager::getInstance().subscribe("woniu/status");
    } else {
        LOG_E("MQTT", "❌ 连接被拒绝！码: %d, 原因: %s", rc, mosquitto_connack_string(rc));
    }
}

void MqttManager::on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    std::string topic(msg->topic);
    std::string payload((char*)msg->payload, msg->payloadlen);

    // 只有 DEBUG 级别才打印原始负载，防止正式运行日志太杂
    LOG_D("MQTT", "收到消息 [%s]: %s", topic.c_str(), payload.c_str());

    cJSON *root = cJSON_Parse(payload.c_str());
    if (!root) {
        LOG_E("MQTT", "JSON 解析失败！Payload: %s", payload.c_str());
        return;
    }

    if (topic == "woniu/sensor") {
        SensorData s_data;
        if (cJSON_HasObjectItem(root, "temp")) s_data.temp = cJSON_GetObjectItem(root, "temp")->valuedouble;
        if (cJSON_HasObjectItem(root, "humi")) s_data.humi = cJSON_GetObjectItem(root, "humi")->valuedouble;
        if (cJSON_HasObjectItem(root, "light")) s_data.light = cJSON_GetObjectItem(root, "light")->valuedouble;
        if (cJSON_HasObjectItem(root, "ir")) s_data.ir_val = cJSON_GetObjectItem(root, "ir")->valueint;

        SmartGateway::getInstance().updateSensor(s_data);
    }
    else if (topic == "woniu/status") {
        DeviceStatus d_ctrl = SmartGateway::getInstance().getControlState();
        bool changed = false;

        // 检查各字段
        if (cJSON_HasObjectItem(root, "led_on")) { d_ctrl.led_on = cJSON_GetObjectItem(root, "led_on")->valueint; changed = true; }
        if (cJSON_HasObjectItem(root, "led_br")) { d_ctrl.led_br = cJSON_GetObjectItem(root, "led_br")->valueint; changed = true; }
        if (cJSON_HasObjectItem(root, "motor_on")) { d_ctrl.motor_on = cJSON_GetObjectItem(root, "motor_on")->valueint; changed = true; }
        if (cJSON_HasObjectItem(root, "motor_sp")) { d_ctrl.motor_sp = cJSON_GetObjectItem(root, "motor_sp")->valueint; changed = true; }
        if (cJSON_HasObjectItem(root, "motor_dir")) { d_ctrl.motor_dir = cJSON_GetObjectItem(root, "motor_dir")->valueint; changed = true; }
        if (cJSON_HasObjectItem(root, "buzzer")) { d_ctrl.buzzer = cJSON_GetObjectItem(root, "buzzer")->valueint; changed = true; }

        if (changed) {
            SmartGateway::getInstance().updateControl(d_ctrl);
            LOG_I("MQTT", "硬件状态已通过服务器反馈完成同步");
        }
    }

    cJSON_Delete(root);
}

void MqttManager::on_disconnect(struct mosquitto *m, void *obj, int rc) {
    if (rc == 0) {
        LOG_I("MQTT", "ℹ️ 客户端已主动断开连接");
    } else {
        LOG_E("MQTT", "❌ 意外断开连接！错误码: %d (%s)", rc, mosquitto_strerror(rc));
        LOG_W("MQTT", "系统将根据策略自动尝试重连...");
    }
}

void MqttManager::stop() {
    if (mosq) {
        LOG_I("MQTT", "正在向服务器发送 DISCONNECT 请求...");
        mosquitto_disconnect(mosq);        
        mosquitto_loop_stop(mosq, false);   
        LOG_I("MQTT", "网络线程已停止");
    }
}