#include "MqttManager.hpp"
#include "cJSON.h"
#include "SmartGateway.hpp" // 引用网关单例，方便收到消息后直接控制硬件

MqttManager::MqttManager() {
    mosquitto_lib_init();
}

MqttManager::~MqttManager() {
    if (mosq) mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

bool MqttManager::init(const std::string& id, const std::string& host, int port) {
    mosq = mosquitto_new(id.c_str(), true, this);
    if (!mosq) return false;

    // 设置回调
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    int rc = mosquitto_connect(mosq, host.c_str(), port, 60);
    return (rc == MOSQ_ERR_SUCCESS);
}

bool MqttManager::publish(const std::string& topic, const std::string& message) {
    int rc = mosquitto_publish(mosq, nullptr, topic.c_str(), message.length(), message.c_str(), 1, false);
    return (rc == MOSQ_ERR_SUCCESS);
}

bool MqttManager::subscribe(const std::string& topic) {
    int rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 1);
    return (rc == MOSQ_ERR_SUCCESS);
}

void MqttManager::on_connect(struct mosquitto *m, void *obj, int rc) {
    if (rc == 0) {
        std::cout << "[MQTT] 成功连接至服务器" << std::endl;
        // 连接成功后可以自动订阅控制主题
        MqttManager::getInstance().subscribe("woniu/control");
    }
}

void MqttManager::on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    std::string topic(msg->topic);
    std::string payload((char*)msg->payload, msg->payloadlen);
    
    std::cout << "[MQTT] 收到话题 " << topic << " 的消息: " << payload << std::endl;

    // 如果收到控制指令，直接调用网关单例控制硬件
    if (topic == "woniu/control") {
        // 这里可以解析 JSON 并调用 SmartGateway::getInstance().handle_control_logic()
    }
}
