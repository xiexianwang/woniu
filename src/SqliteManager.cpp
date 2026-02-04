#include "SqliteManager.hpp"

SqliteManager::SqliteManager() {}

SqliteManager::~SqliteManager() {
    if (db) {
        sqlite3_close(db);
        std::cout << "[SQLite] 数据库已安全关闭" << std::endl;
    }
}

bool SqliteManager::init(const std::string& dbPath) {
    // 1. 打开或创建数据库文件
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[SQLite] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 2. 创建传感器数据表（如果不存在）
    const char* createTableSql = 
        "CREATE TABLE IF NOT EXISTS sensors_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "source TEXT,"
        "temperature REAL,"
        "humidity REAL,"
        "timestamp INTEGER"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "[SQLite] 建表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    std::cout << "[SQLite] 数据库初始化成功: " << dbPath << std::endl;
    return true;
}

bool SqliteManager::insertSensorData(const Stm32SensorData& data) {
    if (!data.is_valid) return false;

    std::lock_guard<std::mutex> lock(db_mutex); // 锁定数据库操作

    // 3. 使用预处理语句防止 SQL 注入并提高效率
    const char* insertSql = "INSERT INTO sensors_data (source, temperature, humidity, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // 绑定解析出的数据
    sqlite3_bind_text(stmt, 1, data.source.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, data.temperature);
    sqlite3_bind_double(stmt, 3, data.humidity);
    sqlite3_bind_int64(stmt, 4, data.data_time);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success) {
        std::cout << "[SQLite] 成功记录来自 " << data.source << " 的数据" << std::endl;
    }
    return success;
}
