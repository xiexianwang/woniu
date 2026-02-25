#ifndef SMART_GATEWAY_H
#define SMART_GATEWAY_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "mongoose.h"
#include <cJSON.h>
#include "Logger.h"
#include "DataType.h"
#include "MqttManager.h"
#include "CameraManager.h"
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

    // --- 传感器专用接口 ---
    void updateSensor(const SensorData& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sensors = data; // 只更新传感器，不碰控制状态
    }
                                                                                
    // --- 控制状态专用接口 ---
    void updateControl(const DeviceStatus& ctrl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_controls = ctrl; // 只更新开关，不碰传感器
    }
    
    // --- 获取部分状态 ---
    DeviceStatus getControlState() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_controls;
    }

    // --- 获取全部数据 (给 Web API 用) ---
    // 为了让 index.html 方便，我们在这里把两个结构体合并打包
    void getAllStatus(SensorData &s, DeviceStatus &c) {
        std::lock_guard<std::mutex> lock(m_mutex);
        s = m_sensors;
        c = m_controls;
    }
private:
    SmartGateway();
    ~SmartGateway();

    // Mongoose 回调逻辑
    static void ev_handler(struct mg_connection *c, int ev, void *ev_data);
    
    // ==========================================
    // HTTP API 路由处理函数声明
    // ==========================================

    // 1. 获取设备状态 (GET /api/status)
    // 功能：返回传感器数据(温湿度/光照)和设备开关状态给前端
    void handle_status(struct mg_connection *c);

    // 2. 设备控制 (POST /api/control)
    // 功能：接收 Web/Qt 发来的 JSON 指令，直接转发 MQTT 给硬件 (不修改本地状态)
    void handle_control(struct mg_connection *c, struct mg_http_message *hm);

    // 3. 摄像头-抓拍 (GET /api/camera/snapshot)
    // 功能：截取当前视频流的一帧并保存为图片
    void handle_snapshot(struct mg_connection *c);

    // 4. 摄像头-开始录像 (GET /api/camera/start_record)
    // 功能：初始化 VideoWriter，开始将视频流写入文件
    void handle_start_record(struct mg_connection *c);

    // 5. 摄像头-停止录像 (GET /api/camera/stop_record)
    // 功能：释放 VideoWriter，保存视频文件
    void handle_stop_record(struct mg_connection *c);
    // 硬件抽象层：读取 RK3568 系统温度
    float read_cpu_temperature();

    struct mg_mgr mgr{};
    bool is_running{false};

    SensorData m_sensors;  // 存温度、湿度
    DeviceStatus m_controls; // 存开关、电机
    std::mutex m_mutex;
};

#endif
