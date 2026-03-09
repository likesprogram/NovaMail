# 工程根目录（绝对路径，末尾带 /）
ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# 你有哪些子工程就列哪些
SUBPROJECTS := LikeHub Smtp

# 顶层输出目录
OUT_BIN_DIR := $(ROOT)bin
OUT_LIB_DIR := $(ROOT)lib
export OUT_BIN_DIR OUT_LIB_DIR

# 并行 make
MAKE_JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

.PHONY: clean build info dirs install \
        $(SUBPROJECTS) \
        run run-% \
        compile_commands

# 安装 LikeHub
install-LikeHub:
	@$(MAKE) --no-print-directory -C $(ROOT)LikeHub install

install: all
	@echo "==> installing binaries to $(OUT_BIN_DIR)"
	@mkdir -p $(OUT_BIN_DIR)
	@for p in $(SUBPROJECTS); do \
		if [ -d "$(ROOT)$$p/bin" ]; then \
			echo "  -> $$p"; \
			find "$(ROOT)$$p/bin" -type f -perm -111 -exec cp -f {} $(OUT_BIN_DIR)/ \; ; \
		fi \
	done

all: clean install-LikeHub dirs $(SUBPROJECTS)

dirs:
	@mkdir -p $(OUT_BIN_DIR)

# 逐个子工程构建（并行）
$(SUBPROJECTS):
	@echo "==> build [$@]"
	@$(MAKE) --no-print-directory -C $(ROOT)$@ -j$(MAKE_JOBS)

# 统一清理
clean:
	@for p in $(SUBPROJECTS); do \
		echo "==> clean [$${p}]"; \
		$(MAKE) --no-print-directory -C $(ROOT)$${p} clean; \
	done
	@echo "==> removing installed binaries from $(OUT_BIN_DIR) and $(OUT_LIB_DIR)"
	@rm -vrf $(OUT_BIN_DIR)/* $(OUT_LIB_DIR)/libLikeHubStatic.a

# 统一打印信息
info:
	@echo "ROOT        = $(ROOT)"
	@echo "SUBPROJECTS = $(SUBPROJECTS)"
	@echo "MAKE_JOBS   = $(MAKE_JOBS)"
	@for p in $(SUBPROJECTS); do \
		echo "==> info [$${p}]"; \
		$(MAKE) --no-print-directory -C $(ROOT)$${p} info; \
	done

# 运行：make run 默认运行 Smtp
run: run-Smtp

# 运行指定工程：make run-discovery / make run-registry
run-%:
	@echo "==> run [$*]"
	@$(MAKE) --no-print-directory -C $(ROOT)$* run

# 生成 compile_commands.json
build:
	@command -v bear >/dev/null 2>&1 || { echo "bear not found. Install: sudo apt install bear"; exit 1; }
	@rm -f $(ROOT)compile_commands.json
	@echo "==> generating compile_commands.json via bear (top-level)"
	@bear -o $(ROOT)compile_commands.json $(MAKE) --no-print-directory all
