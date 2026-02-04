#include "SmartGateway.hpp"

int main() {
    // 启动网关服务，监听 80 端口
    SmartGateway::getInstance().start("80");
    return 0;
}
