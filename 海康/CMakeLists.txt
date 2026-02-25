cmake_minimum_required(VERSION 3.10)
project(woniu_app)

set(CMAKE_CXX_STANDARD 14)

# 【核心：加回这一行】告诉 CMake 别在系统里瞎找，直接去这里找 OpenCV
set(OpenCV_DIR "/home/xxw/source/rk356x_linux/buildroot/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/cmake/opencv4")

# 现在 find_package 就能找到 4.5.5 版本了
find_package(OpenCV REQUIRED)

# 2. 头文件路径优化
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    # 指向 Buildroot 里的 cjson 头文件
    /home/xxw/source/rk356x_linux/buildroot/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/cjson
)

# 3. 源文件定义 (确保 src/ 下的文件你都从 Timeline 恢复了)
set(SOURCES 
    main.cpp 
    src/SmartGateway.cpp 
    src/MqttManager.cpp 
    src/SqliteManager.cpp
    src/CameraManager.cpp
    src/JsonParser.cpp
    src/mongoose.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 4. 链接库
target_link_libraries(${PROJECT_NAME} 
    ${OpenCV_LIBS}
    mosquitto sqlite3 cjson pthread dl
)