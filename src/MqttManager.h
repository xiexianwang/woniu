#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include <string>
#include <iostream>
#include <mosquitto.h>
#include <memory>

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

    // 开启后台循环处理网络事务
    void loopStart() { mosquitto_loop_start(mosq); }

private:
    MqttManager();
    ~MqttManager();

    // MQTT 回调函数
    static void on_connect(struct mosquitto *m, void *obj, int rc);
    static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg);

    struct mosquitto *mosq = nullptr;
};

#endif
