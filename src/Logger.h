// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>

// 定义日志级别
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// 简单的日志封装
class Logger {
public:
    static void log(LogLevel level, const std::string& module, const std::string& message) {
        // 这里加个锁，防止多线程打印时文字乱序
        static std::mutex logMutex; 
        std::lock_guard<std::mutex> lock(logMutex);

        std::string levelStr;
        switch(level) {
            case DEBUG:   levelStr = "[DEBUG]"; break;
            case INFO:    levelStr = "[INFO] "; break;
            case WARNING: levelStr = "[WARN] "; break;
            case ERROR:   levelStr = "[ERROR]"; break;
        }

        // 格式：[级别] [模块名] 消息
        // 例如：[INFO] [Serial] Open port success
        std::cout << levelStr << " [" << module << "] " << message << std::endl;
    }
};

// 定义宏，方便调用，同时能在后期轻松替换成其他复杂的库
#define LOG_D(module, msg) Logger::log(DEBUG, module, msg)
#define LOG_I(module, msg) Logger::log(INFO, module, msg)
#define LOG_W(module, msg) Logger::log(WARNING, module, msg)
#define LOG_E(module, msg) Logger::log(ERROR, module, msg)

#endif
