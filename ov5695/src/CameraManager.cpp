#include "CameraManager.h"
#include "Logger.h"
#include <cstdlib>
#include <unistd.h>
#include <iostream>

// 现在不再需要 open() 里的 OpenCV 线程了，除非你要做 AI 分析
bool CameraManager::open() {
    if (is_running) return true;
    
    LOG_I("Camera", "正在启动底层流媒体服务 (调用外部脚本)...");

    // 1. 执行你写好的启动脚本 (请确保脚本里有 & 符号让服务后台运行)
    int ret = std::system("/bin/bash /woniu/start_stream.sh");
    
    // 2. 检查脚本是否成功执行
    if (ret != 0) {
        // 如果脚本执行失败，打印错误日志 (注意：部分系统环境下 system 返回值需要宏解析，这里仅做简单非 0 判定)
        LOG_E("Camera", ("流媒体脚本启动可能出现异常，错误码: " + std::to_string(ret)).c_str());
        // 如果失败就不让状态变成 true，直接 return false; (按需开启)
        // return false; 
    }

    // 顺便帮你把日志里的 RTSP 改成了现在超牛的 WebRTC 模式
    LOG_I("Camera", "摄像头管理模块初始化成功 (WebRTC 超低延迟模式)");
    is_running = true;
    return true;
}

void CameraManager::stop() {
    if (!is_running) return;
    if (is_recording) stopRecord();

    LOG_I("Camera", "正在关闭底层流媒体服务...");

    // 1. 暴力清理所有推流相关的后台进程 (比单独写个脚本更直接)
    // 加上 2>/dev/null 是为了防止如果没有这些进程时，终端乱弹报错信息
    std::system("killall -9 gst-launch-1.0 mediamtx rkaiq_3A_server 2>/dev/null");

    is_running = false;
    LOG_I("Camera", "摄像头管理模块已关闭");
}

// ==========================================
// 🔴 开始录像
// ==========================================
bool CameraManager::startRecord(const std::string& savePath) {
    if (is_recording) {
        LOG_W("Camera", "录像已在运行中");
        return false;
    }

    // 依然从本地 RTSP 端口拉流。加上 -y 参数自动覆盖同名文件，防止 FFmpeg 卡在询问处
    std::string cmd = "ffmpeg -y -rtsp_transport tcp -i rtsp://127.0.0.1:8554/cam -c copy " + savePath + " > /dev/null 2>&1 & echo $! > /tmp/ffmpeg_rec.pid";
    
    LOG_I("Camera", "正在拉起 FFmpeg 零拷贝录像进程...");
    
    if (std::system(cmd.c_str()) == 0) {
        is_recording = true;
        // 注意：C++ 中 LOG_I 如果是用类似 printf 的格式化，传 std::string 需要加 .c_str()
        LOG_I("Camera", "🚀 录像启动成功，文件: %s", savePath.c_str());
        return true;
    }
    
    LOG_E("Camera", "录像进程拉起失败！");
    return false;
}

// ==========================================
// ⏹ 停止录像
// ==========================================
void CameraManager::stopRecord() {
    if (!is_recording) return;

    LOG_I("Camera", "正在停止录像，写入 MP4 尾部索引...");

    // 发送 SIGINT (-2) 信号给 FFmpeg，让它正常结束并写入 moov 头（否则 MP4 无法播放）
    std::system("kill -2 $(cat /tmp/ffmpeg_rec.pid) 2>/dev/null");
    
    // 给 FFmpeg 1 秒钟的时间保存文件
    sleep(1); 
    
    std::system("rm -f /tmp/ffmpeg_rec.pid");
    is_recording = false;
    LOG_I("Camera", "✅ 录像已安全停止并保存");
}

// ==========================================
// 📸 瞬间抓拍 (新增)
// ==========================================
bool CameraManager::takeSnapshot(const std::string& savePath) {
    LOG_I("Camera", "正在抓拍实时画面...");

    // 魔法指令：从 RTSP 流中抽取 1 帧 (-vframes 1)，并以较高质量 (-q:v 2) 存为 JPEG
    std::string cmd = "ffmpeg -y -rtsp_transport tcp -i rtsp://127.0.0.1:8554/cam -vframes 1 -q:v 2 " + savePath + " > /dev/null 2>&1";
    
    if (std::system(cmd.c_str()) == 0) {
        LOG_I("Camera", "📸 照片抓拍成功，保存至: %s", savePath.c_str());
        return true;
    }
    
    LOG_E("Camera", "照片抓拍失败！");
    return false;
}