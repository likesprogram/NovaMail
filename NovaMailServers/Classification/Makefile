# Makefile 所在目录（绝对路径，末尾带 /）
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRC_DIR = $(MAKEFILE_DIR)src
OBJ_DIR = $(MAKEFILE_DIR)obj
BIN_DIR = $(MAKEFILE_DIR)bin
INCLUDE_DIR = -I $(MAKEFILE_DIR)include
INCLUDE_DIR += -I $(MAKEFILE_DIR)../include

# 链接选项
LDFLAGS := -L $(MAKEFILE_DIR)../lib
LDFLAGS += -Wl,-rpath,'$$ORIGIN'
LDLIBS = -Wl,--start-group
LDLIBS += $(MAKEFILE_DIR)../lib/libLikesProgramStatic.a
LDLIBS += $(MAKEFILE_DIR)../lib/libLikeHubStatic.a
LDLIBS += $(MAKEFILE_DIR)../lib/libmysqlcppconnx-static.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-auth.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-cal.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-common.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-compression.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-event-stream.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-http.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-io.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-mqtt.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-s3.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-c-sdkutils.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-checksums.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-cpp-sdk-core.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-cpp-sdk-s3.a
LDLIBS += $(MAKEFILE_DIR)../lib/libaws-crt-cpp.a
LDLIBS += $(MAKEFILE_DIR)../lib/libs2n.a
LDLIBS += -Wl,--end-group
LDLIBS += -lonnxruntime -lcurl -lssl -lcrypto -lz -lresolv -lpthread -lrt -ldl

# 编译选项
CXX = g++-11 -O0 -fno-omit-frame-pointer
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror
CXXFLAGS += $(INCLUDE_DIR)
# 自动生成依赖 .d
CXXFLAGS += -MMD -MP

# 递归找所有 .cpp
SRCS := $(shell find $(SRC_DIRS) -type f -name '*.cpp')
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# 输出文件
TARGET = $(BIN_DIR)/NovaClassification.out
# ONNX Runtime
ONNXRUNTIME_SO := $(MAKEFILE_DIR)../lib/libonnxruntime.so.1

# Phony targets
.PHONY: all clean run info dirs copy-onnx

all: dirs copy-onnx $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

copy-onnx: dirs
	@if [ ! -f "$(ONNXRUNTIME_SO)" ]; then \
		echo "Error: $(ONNXRUNTIME_SO) not found"; \
		exit 1; \
	fi
	@cp -f $(ONNXRUNTIME_SO) $(BIN_DIR)/

info:
	@echo "CXX            = $(CXX)"
	@echo "MAKEFILE_DIR   = $(MAKEFILE_DIR)"
	@echo "SRC_DIR        = $(SRC_DIR)"
	@echo "OBJ_DIR        = $(OBJ_DIR)"
	@echo "BIN_DIR        = $(BIN_DIR)"
	@echo "TARGET         = $(TARGET)"
	@echo "ONNXRUNTIME_SO = $(ONNXRUNTIME_SO)"
	@echo "SRCS           = $(words $(SRCS)) files"
	@echo "CXXFLAGS       = $(CXXFLAGS)"
	@echo "LDFLAGS        = $(LDFLAGS)"
	@echo "LDLIBS         = $(LDLIBS)"

# 链接
$(TARGET): $(OBJS) $(LIB)
	@mkdir -p $(dir $@)
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# 编译：src/a/b.cpp -> obj/a/b.o
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

# 自动依赖
run: all
	@$(TARGET)

clean:
	@rm -vrf $(OBJ_DIR) $(BIN_DIR)