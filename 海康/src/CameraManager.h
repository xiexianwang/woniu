#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <opencv2/opencv.hpp>
#include <mutex>
#include <thread>
#include <string>

class CameraManager {
public:
    static CameraManager& getInstance() {
        static CameraManager instance;
        return instance;
    }
    bool open();
    bool takeSnapshot(const std::string& savePath);
    bool startRecord(const std::string& savePath);
    void stopRecord();
    bool getFrame(cv::Mat& outFrame);
    void stop();

private:
    CameraManager() : is_running(false), is_recording(false) {}
    void run(); 

    cv::VideoCapture cap;
    cv::Mat current_frame;
    std::mutex frame_mutex;
    bool is_running;
    bool is_recording;
    std::thread capture_thread;
};
#endif