# ======= 1. 基础配置 =======
BUILD_DIR   = build
TARGET_IP   = 192.168.1.6
TARGET_PASS = www.xxw123.com
TARGET_USER = root
CROSS_PATH  = /home/xxw/source/rk356x_linux/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-

# ======= 支持位置参数 (LOG级别) =======
ifeq ($(firstword $(MAKECMDGOALS)),run)
  LOG_VAL := $(word 2,$(MAKECMDGOALS))
  LOG_VAL := $(if $(LOG_VAL),$(LOG_VAL),INFO)
  $(eval $(LOG_VAL):;@:)
endif

# ======= 2. 定义命令快捷方式 =======
SSH_OPTS    = -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
SSH_CMD     = sshpass -p $(TARGET_PASS) ssh $(SSH_OPTS)
SCP_CMD     = sshpass -p $(TARGET_PASS) scp $(SSH_OPTS)

.PHONY: all clean deploy run

all:
	@mkdir -p $(BUILD_DIR)
	@echo "--- 正在生成 Makefile (默认级别: $(LOG_VAL)) ---"
	@cd $(BUILD_DIR) && cmake \
		-DCMAKE_CXX_COMPILER=$(CROSS_PATH)g++ \
		-DCMAKE_C_COMPILER=$(CROSS_PATH)gcc \
		-DCMAKE_CXX_FLAGS="-DDEFAULT_LOG_LEVEL=$(LOG_VAL)" \
		..
	@make -C $(BUILD_DIR) -j16

# ======= 🌟 3. 部署路径全面更新 =======
deploy: all
	@echo "--- 正在推送程序、网页与脚本 ---"
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "killall -2 woniu_app 2>/dev/null || true"
	@sleep 2
	# 创建主目录和前端静态文件目录
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "mkdir -p /woniu/web /woniu/photos /woniu/videos"
	
	# 推送主程序
	@$(SCP_CMD) build/woniu_app $(TARGET_USER)@$(TARGET_IP):/woniu/woniu_app
	
	# 推送网页 (放入 /woniu/web)
	@$(SCP_CMD) index.html $(TARGET_USER)@$(TARGET_IP):/woniu/web/index.html
	
	# 推送启动脚本
	@$(SCP_CMD) start_stream.sh $(TARGET_USER)@$(TARGET_IP):/woniu/start_stream.sh 2>/dev/null || true
	
	# 赋予可执行权限
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "chmod +x /woniu/woniu_app /woniu/start_stream.sh && sync"
	@echo "部署完成！"

# ======= 🌟 4. 极简版一键运行目标 =======
run: deploy
	@echo "--- 正在清理旧主程序 ---"
	# 现在不需要在 Makefile 里杀推流进程了，因为 start_stream.sh 里已经写了 killall
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "killall -2 woniu_app 2>/dev/null || true"

	@echo "--- 正在启动智能网关主程序 (级别: $(LOG_VAL)) ---"
	# 运行主程序，C++ 内部会自动调用 /woniu/start_stream.sh 拉起推流
	@$(SSH_CMD) $(TARGET_USER)@$(TARGET_IP) "export LD_LIBRARY_PATH=/usr/lib; /woniu/woniu_app $(LOG_VAL)"

clean:
	rm -rf $(BUILD_DIR)