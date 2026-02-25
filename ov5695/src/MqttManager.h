#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <string>
#include <iostream>
#include <mosquitto.h>
#include <memory>
#include "Logger.h"

// MQTT 管理器单例
class MqttManager {
public:
    // Meyers' Singleton: C++14 线程安全初始化
    static MqttManager& getInstance() {
        static MqttManager instance;
        return instance;
    }

    // 禁用拷贝
    MqttManager(const MqttManager&) = delete;
    MqttManager& operator=(const MqttManager&) = delete;

    // 基础操作接口
    bool init(const std::string& id, const std::string& host, int port = 1883);
    bool publish(const std::string& topic, const std::string& message);
    bool subscribe(const std::string& topic);
    void stop();
    static void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str) {
    // 直接打印库内部的报错信息
    fprintf(stderr, "[Mosquitto Internal] %s\n", str);
    }
private:
    MqttManager();
    ~MqttManager();

    // MQTT 回调函数
    static void on_connect(struct mosquitto *m, void *obj, int rc);
    static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg);
    static void on_disconnect(struct mosquitto *m, void *obj, int rc);
    struct mosquitto *mosq = nullptr;
};

#endif
