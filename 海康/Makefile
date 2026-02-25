# ======= 1. 基础配置 =======
BUILD_DIR   = build
TARGET_IP   = 192.168.1.6
TARGET_PASS = www.xxw123.com
TARGET_USER = root
CROSS_PATH  = /home/xxw/source/rk356x_linux/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-

# ======= 🌟 核心修改：支持位置参数 =======
# 如果第一个目标是 run
ifeq ($(firstword $(MAKECMDGOALS)),run)
  # 取出第二个单词作为日志级别
  LOG_VAL := $(word 2,$(MAKECMDGOALS))
  # 如果用户没写参数，默认 DEBUG
  LOG_VAL := $(if $(LOG_VAL),$(LOG_VAL),INFO)
  # 关键：定义一个空的规则，防止 make 报错说 "No rule to make target 'INFO'"
  $(eval $(LOG_VAL):;@:)
endif

# ======= 2. 定义命令快捷方式 =======
SSH_OPTS    = -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
SSH_CMD     = sshpass -p $(TARGET_PASS) ssh $(SSH_OPTS)
SCP_CMD     = sshpass -p $(TARGET_PASS) scp $(SSH_OPTS)

.PHONY: all clean deploy run

# 编译 (依然支持编译时注入默认值)
all:
	@mkdir -p $(BUILD_DIR)
	@echo "--- 正在生成 Makefile (默认级别: $(LOG_VAL)) ---"
	@cd $(BUILD_DIR) && cmake \
		-DCMAKE_CXX_COMPILER=$(CROSS_PATH)g++ \
		-DCMAKE_C_COMPILER=$(CROSS_PATH)gcc \
		-DCMAKE_CXX_FLAGS="-DDEFAULT_LOG_LEVEL=$(LOG_VAL)" \
		..
	@make -C $(BUILD_DIR) -j16

# 部署
deploy: all
	@echo "--- 正在推送程序与网页 ---"
	@$(SCP_CMD) build/woniu_app $(TARGET_USER)@$(TARGET_IP):/woniu_app
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "mkdir -p /usr/www"
	@$(SCP_CMD) index.html $(TARGET_USER)@$(TARGET_IP):/usr/www/index.html
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "chmod +x /woniu_app && sync"
	@echo "部署完成！"

# ======= 3. 一键运行 =======
# 这里的用法现在支持：make run INFO 或 make run DEBUG
run: deploy
	@echo "--- 检查并自动启动 mjpg_streamer 推流 ---"
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) \
		"pgrep mjpg_streamer > /dev/null || \
		(mjpg_streamer -i 'input_uvc.so -d /dev/video9 -r 1920x1080 -f 30' \
		-o 'output_http.so -w /usr/www -p 8081' &)"
	
	@echo "--- 正在板子上启动主程序 (级别: $(LOG_VAL)) ---"
	# 🌟 核心修改：将解析出来的 LOG_VAL 传给 argv[1]
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "export LD_LIBRARY_PATH=/usr/lib; /woniu_app $(LOG_VAL)"

clean:
	rm -rf $(BUILD_DIR)