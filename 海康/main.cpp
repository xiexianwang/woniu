#include "SmartGateway.h"
#include "Logger.h"
#include "CameraManager.h"
#include "MqttManager.h"
#include <csignal>
#include <algorithm>
#include <unistd.h>
#include <ctime>

// ======= MQTT 配置宏 =======
#define MQTT_BROKER_IP   "broker.emqx.io"
#define MQTT_BROKER_PORT 1883

// 辅助函数：解析字符串为 LogLevel 枚举
LogLevel parseLogLevel(std::string levelStr) {
    std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::toupper);

    if (levelStr == "DEBUG")   return DEBUG;
    if (levelStr == "INFO")    return INFO;
    if (levelStr == "WARNING" || levelStr == "WARN") return WARNING;
    if (levelStr == "ERROR")   return ERROR;

    // 如果输入错误，返回 DEBUG 并提示 (或者根据需求设为 INFO)
    std::cout << "⚠️ 无效的日志级别: " << levelStr << "，已自动设为 DEBUG" << std::endl;
    return DEBUG;
}

// 信号处理函数：负责“优雅地关机”
void signalHandler(int signum) {
    LOG_W("System", "捕获到中断信号 (%d)，准备安全退出...", signum);
    
    LOG_I("Camera", "正在关闭摄像头保活线程...");
    CameraManager::getInstance().stop();
    
    LOG_I("MQTT", "正在断开 MQTT 连接并刷新缓冲区...");
    MqttManager::getInstance().stop();
    
    usleep(500000); 
    
    LOG_I("System", "✅ 所有资源已释放，进程结束。");
    exit(signum);
}

int main(int argc, char* argv[]) {
    // 1. 设置一个默认值
    LogLevel targetLevel = INFO;
    // --- 2. 命令行参数覆盖 ---
    // 如果用户执行了 ./woniu_app INFO，则 argv[1] 会覆盖上面的默认值
    if (argc >= 2) {
        targetLevel = parseLogLevel(argv[1]);
    }

    // --- 3. 应用日志级别 ---
    Logger::set_level(targetLevel);

    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // --- 4. 打印启动横幅与状态 ---
    LOG_I("System", "========================================");
    LOG_I("System", "   RK3568 智能网关系统启动中... (2026)");
    LOG_I("System", "========================================");
    
    // 🌟 严格按照你的要求打印日志级别
    LOG_I("System", "当前日志过滤级别已设为: %d (0:DEBUG, 1:INFO, 2:WARN, 3:ERROR)", (int)targetLevel);

    // --- 5. 初始化各模块 ---
    // 1. 生成带时间戳的唯一 Client ID
    // 格式如：rk3568_gateway_1708854482
    std::string clientId = "rk3568_gateway_" + std::to_string(time(NULL));

    // 1. 初始化 MQTT
    LOG_I("MQTT", "正在尝试连接远程服务器 (%s)...", MQTT_BROKER_IP);
    LOG_I("MQTT", "本地客户端 ID: %s", clientId.c_str());

    // 使用宏和动态生成的 clientId
    if (!MqttManager::getInstance().init(clientId, MQTT_BROKER_IP, MQTT_BROKER_PORT)) {
        LOG_E("MQTT", "MQTT 初始化失败，请检查网络设置。");
        return -1;
    }

    LOG_I("MQTT", "✅ MQTT 服务已成功接入云端 (ID: %s)", clientId.c_str());

    // 2. 初始化摄像头
    LOG_I("Camera", "正在连接 mjpg-streamer 推流服务...");
    if (!CameraManager::getInstance().open()) {
        LOG_E("System", "致命错误：无法打开摄像头！请确认 mjpg-streamer 是否运行。");
        return -1;
    }
    LOG_I("Camera", "✅ 摄像头保活/预览服务已就绪。");

    

    // 3. 启动 Web 网关
    LOG_I("Web", "正在开启 HTTP API 服务，端口: 8080");
    LOG_I("Web", "静态网页根目录: /usr/www");
    
    // 进入 Mongoose 事件循环
    SmartGateway::getInstance().start("8080");

    return 0;
}