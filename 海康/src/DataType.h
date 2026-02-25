#pragma once

// 1. 设备控制状态 (Output) - 对应网页控制区
struct DeviceStatus {
    bool led_on = false;    // 指示灯开关
    int led_br = 0;        // 指示灯亮度
    
    bool motor_on = false;  // 电机控制开关
    int motor_sp = 0;       // 电机速度设置
    int motor_dir = 0;      // 电机方向设置
    
    bool buzzer = false;    // 蜂鸣器开关
};

// 2. 传感器状态 (Input) - 对应网页数据区
struct SensorData {
    float temp = 0.0f;     // 温度
    float humi = 0.0f;     // 湿度
    float light = 0.0f;    // 光照强度
    
    int ir_val = 0;        // 红外传感器值
    
};