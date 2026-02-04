#ifndef SQLITE_MANAGER_HPP
#define SQLITE_MANAGER_HPP

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include "JsonParser.hpp" // 引用之前的解析器结构体

class SqliteManager {
public:
    // C++14 线程安全单例接口
    static SqliteManager& getInstance() {
        static SqliteManager instance;
        return instance;
    }

    // 禁用拷贝
    SqliteManager(const SqliteManager&) = delete;
    SqliteManager& operator=(const SqliteManager&) = delete;

    // 初始化数据库与建表
    bool init(const std::string& dbPath = "woniu_data.db");
    
    // 插入传感器数据
    bool insertSensorData(const Stm32SensorData& data);

private:
    SqliteManager();
    ~SqliteManager();

    sqlite3* db = nullptr;
    std::mutex db_mutex; // 确保在多线程环境下的写安全
};

#endif
