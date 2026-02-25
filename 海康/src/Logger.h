#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstdarg>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>

// 定义日志级别
enum LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };

class Logger {
public:
    // 🌟 核心新增：设置全局日志级别
    // 例如：设置成 INFO，则所有的 DEBUG 日志都不会打印
    static void set_level(LogLevel level) {
        current_level() = level;
    }

    static void log_fmt(LogLevel level, const char* module, const char* fmt, ...) {
        // 🌟 核心过滤逻辑：当前级别低于设定级别，直接跳过
        if (level < current_level()) return;

        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex);

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm* parts = std::localtime(&now_c);

        char message[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);

        std::string levelStr;
        std::string colorStr;
        switch(level) {
            case DEBUG:   levelStr = "[DEBUG]"; colorStr = "\033[36m"; break; // 青色
            case INFO:    levelStr = "[INFO ]"; colorStr = "\033[32m"; break; // 绿色
            case WARNING: levelStr = "[WARN ]"; colorStr = "\033[33m"; break; // 黄色
            case ERROR:   levelStr = "[ERROR]"; colorStr = "\033[31m"; break; // 红色
        }

        std::stringstream ss;
        ss << "[" << std::put_time(parts, "%Y-%m-%d %H:%M:%S") << "." 
           << std::setfill('0') << std::setw(3) << ms.count() << "] "
           << levelStr << " [" << module << "] " << message;
        std::string logLine = ss.str();

        // 打印到终端
        std::cout << colorStr << logLine << "\033[0m" << std::endl;

        // 写入到指定目录
        write_to_file(logLine);
    }

private:
    // 🌟 辅助函数：利用静态局部变量存储当前级别，默认 DEBUG
    static LogLevel& current_level() {
        static LogLevel m_level = DEBUG;
        return m_level;
    }

    static void write_to_file(const std::string& line) {
        // 🌟 路径严格锁定为 /woniu
        const std::string dirPath = "/woniu";
        const std::string logPath = dirPath + "/woniu_app.log";

        if (access(dirPath.c_str(), F_OK) != 0) {
            std::string makeDirCmd = "mkdir -p " + dirPath;
            if (system(makeDirCmd.c_str()) != 0) {
                save_raw("woniu_app_backup.log", line);
                return;
            }
        }

        struct stat stat_buf;
        if (stat(logPath.c_str(), &stat_buf) == 0) {
            if (stat_buf.st_size > 100 * 1024 * 1024) {
                std::ofstream ofs(logPath, std::ios::trunc);
                ofs << "[SYSTEM] 日志文件超过100MB，执行回滚重置" << std::endl;
                ofs.close();
            }
        }

        save_raw(logPath, line);
    }

    static void save_raw(const std::string& path, const std::string& line) {
        std::ofstream logFile(path, std::ios::app);
        if (logFile.is_open()) {
            logFile << line << std::endl;
            logFile.close();
        }
    }
};

// 宏定义
#define LOG_D(module, fmt, ...) Logger::log_fmt(DEBUG, module, fmt, ##__VA_ARGS__)
#define LOG_I(module, fmt, ...) Logger::log_fmt(INFO, module, fmt, ##__VA_ARGS__)
#define LOG_W(module, fmt, ...) Logger::log_fmt(WARNING, module, fmt, ##__VA_ARGS__)
#define LOG_E(module, fmt, ...) Logger::log_fmt(ERROR, module, fmt, ##__VA_ARGS__)

#endif