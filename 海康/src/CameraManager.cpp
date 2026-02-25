#include "CameraManager.h"
#include "Logger.h"  // 引入你刚写的日志头文件
#include <iostream>
#include <cstdlib>
#include <unistd.h>

bool CameraManager::open() {
    if (is_running) return true;

    // 依然读取 mjpg-streamer，用于给 Web 提供预览和拍照
    std::string stream_url = "http://127.0.0.1:8081/?action=stream";
    cap.open(stream_url);
    
    if (!cap.isOpened()) {
        LOG_E("Camera", "摄像头预览流连接失败！URL: %s", stream_url.c_str());
        return false;
    }

    is_running = true;
    capture_thread = std::thread(&CameraManager::run, this);
    
    LOG_I("Camera", "OpenCV 预览/保活线程启动成功");
    return true;
}

void CameraManager::stop() {
    if (!is_running) return;

    is_running = false; // 停止 run() 循环
    if (capture_thread.joinable()) {
        capture_thread.join(); // 等待后台线程结束
    }
    
    stopRecord(); // 如果正在录像，确保封口
    
    if (cap.isOpened()) {
        cap.release(); // 释放摄像头连接
    }
    
    LOG_I("Camera", "硬件资源已安全释放");
}

// 预览线程：只负责刷新 current_frame，供网页和拍照使用
void CameraManager::run() {
    cv::Mat temp_frame;
    LOG_D("Camera", "后台拉流循环进入运行状态");

    while (is_running ) {
        if (!cap.read(temp_frame) || temp_frame.empty()) {
            LOG_W("Camera", "读取视频帧失败，尝试重试...");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            temp_frame.copyTo(current_frame);
        }
    }
    LOG_D("Camera", "后台拉流循环已退出");
}

// 🔴 录像：直接拉起 FFmpeg 雇佣兵
bool CameraManager::startRecord(const std::string& savePath) {
    if (is_recording) {
        LOG_W("Camera", "录像任务已在运行中，请勿重复开启");
        return false;
    }
    // 构造指令
    std::string cmd = "ffmpeg -y -i http://127.0.0.1:8081/?action=stream -c:v copy " + savePath + " > /dev/null 2>&1 & echo $! > /tmp/ffmpeg.pid";
    LOG_I("Camera", "正在拉起 FFmpeg 录像进程...");
    
    if (system(cmd.c_str()) == 0) {
        is_recording = true;
        LOG_I("Camera", "🚀 FFmpeg 录像启动成功！保存路径: %s", savePath.c_str());
        return true;
    } else {
        LOG_E("Camera", "FFmpeg 启动失败！请检查系统路径或 mjpg-streamer 状态");
        return false;
    }
}

// 🔴 停止：给 FFmpeg 发送 Ctrl+C 信号
void CameraManager::stopRecord() {
    if (!is_recording) return;

    LOG_I("Camera", "正在停止录像并封口视频文件...");

    // 发送 SIGINT 信号 (kill -2)，让 FFmpeg 正常保存视频索引
    std::string killCmd = "kill -2 $(cat /tmp/ffmpeg.pid) 2>/dev/null";
    system(killCmd.c_str());
    
    // 给一点点时间让 FFmpeg 写入索引表
    usleep(500000); 

    is_recording = false;
    LOG_I("Camera", "✅ 录像已安全停止");
}