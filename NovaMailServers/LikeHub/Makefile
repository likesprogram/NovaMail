# Makefile 所在目录（绝对路径，末尾不带 /）
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRC_DIRS := $(MAKEFILE_DIR)src
OBJ_DIR  := $(MAKEFILE_DIR)obj
BIN_DIR  := $(MAKEFILE_DIR)bin

SDK_DIR  := $(SRC_DIRS)/LikeHubSDK

INCLUDE_DIR  := -I $(MAKEFILE_DIR)../include
INCLUDE_DIR  += -I $(MAKEFILE_DIR)include

# 编译器与参数
CXX      := g++-11
AR       := ar
RANLIB   := ranlib

CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Werror
CXXFLAGS += $(INCLUDE_DIR)
# 自动生成依赖 .d
CXXFLAGS += -MMD -MP

# 链接选项（外部依赖）
LDLIBS := $(MAKEFILE_DIR)../lib/libLikesProgramStatic.a
LDLIBS += $(MAKEFILE_DIR)../lib/libmysqlcppconnx-static.a
LDLIBS += -lssl -lcrypto -lresolv -lpthread

# 输出文件
TARGET    := $(BIN_DIR)/LikeHubServer.out
TARGETLIB := $(BIN_DIR)/libLikeHubStatic.a

# 全部源码
SRCS_ALL := $(shell find $(SRC_DIRS) -type f -name '*.cpp')
# SDK 源码（只进静态库）
SRCS_SDK := $(shell find $(SDK_DIR) -type f -name '*.cpp')
# APP 源码（可执行文件专用：排除 SDK）
SRCS_APP := $(filter-out $(SRCS_SDK),$(SRCS_ALL))

# 生成相对路径（相对 MAKEFILE_DIR），避免 obj//abs/path 这种怪路径
REL_SRCS_SDK := $(patsubst $(MAKEFILE_DIR)%,%,$(SRCS_SDK))
REL_SRCS_APP := $(patsubst $(MAKEFILE_DIR)%,%,$(SRCS_APP))

# 目标对象文件
OBJS_SDK := $(addprefix $(OBJ_DIR)/,$(REL_SRCS_SDK:.cpp=.o))
OBJS_APP := $(addprefix $(OBJ_DIR)/,$(REL_SRCS_APP:.cpp=.o))

# 依赖文件
DEPS_SDK := $(OBJS_SDK:.o=.d)
DEPS_APP := $(OBJS_APP:.o=.d)

# Phony targets
.PHONY: all install clean run info dirs

install: all
	@echo "==> installing binaries to $(OUT_BIN_DIR)"
	@mkdir -p $(OUT_BIN_DIR)
	@cp -f $(TARGET) $(OUT_BIN_DIR)/
	@echo "==> installing binaries to $(OUT_LIB_DIR)"
	@mkdir -p $(OUT_LIB_DIR)
	@cp -f $(TARGETLIB) $(OUT_LIB_DIR)/

all: dirs $(TARGETLIB) $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

info:
	@echo "CXX         = $(CXX)"
	@echo "AR          = $(AR)"
	@echo "INCLUDE_DIR = $(INCLUDE_DIR)"
	@echo "TARGET      = $(TARGET)"
	@echo "TARGETLIB   = $(TARGETLIB)"
	@echo "SRCS_ALL    = $(words $(SRCS_ALL)) files"
	@echo "SRCS_SDK    = $(words $(SRCS_SDK)) files"
	@echo "SRCS_APP    = $(words $(SRCS_APP)) files"
	@echo "CXXFLAGS    = $(CXXFLAGS)"
	@echo "LDLIBS      = $(LDLIBS)"

# SDK -> 静态库
$(TARGETLIB): $(OBJS_SDK)
	@mkdir -p $(dir $@)
	@$(AR) rcs $@ $(OBJS_SDK)
	@$(RANLIB) $@

# APP + 依赖静态库 -> 可执行文件
$(TARGET): $(OBJS_APP) $(TARGETLIB)
	@mkdir -p $(dir $@)
	@$(CXX) $(OBJS_APP) -o $@ $(TARGETLIB) $(LDLIBS)

# obj/xxx.o <- (MAKEFILE_DIR)xxx.cpp
$(OBJ_DIR)/%.o: $(MAKEFILE_DIR)%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Auto deps
-include $(DEPS_SDK)
-include $(DEPS_APP)

run: all
	@sudo $(TARGET)

clean:
	@rm -vrf $(OBJ_DIR) $(BIN_DIR)