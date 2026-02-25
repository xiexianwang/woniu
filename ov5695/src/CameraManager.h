#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

// 1. 包含 OpenCV 核心头文件
#include <opencv2/opencv.hpp> 
#include <opencv2/core.hpp>

// 2. 包含 C++11 标准多线程头文件
#include <mutex>
#include <atomic>
#include <thread>
#include <string>

class CameraManager {
public:
    static CameraManager& getInstance() {
        static CameraManager instance;
        return instance;
    }

    bool open();
    void stop();
    bool takeSnapshot(const std::string& savePath);
    bool startRecord(const std::string& savePath);
    void stopRecord();
    bool getFrame(cv::Mat& outFrame);

private:
    CameraManager() : is_running(false), is_recording(false) {}
    
    // 3. 🌟 注意这里必须是 std:: 
    std::atomic<bool> is_running;
    std::atomic<bool> is_recording;
    
    std::mutex frame_mutex;
    cv::Mat current_frame; 
};

#endif