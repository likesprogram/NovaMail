# Makefile 所在目录（绝对路径，末尾不带 /）
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRC_DIRS = $(MAKEFILE_DIR)src
OBJ_DIR  = $(MAKEFILE_DIR)obj
BIN_DIR  = $(MAKEFILE_DIR)bin
INCLUDE_DIR = -I $(MAKEFILE_DIR)../include
INCLUDE_DIR += -I $(MAKEFILE_DIR)include

# 链接选项
LDLIBS = $(MAKEFILE_DIR)../lib/libLikesProgramStatic.a
LDLIBS += $(MAKEFILE_DIR)../lib/libLikeHubStatic.a
LDLIBS += $(MAKEFILE_DIR)../lib/libmysqlcppconnx-static.a
LDLIBS += -lssl -lcrypto -lresolv -lpthread

# 编译选项
CXX = g++-11
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror
CXXFLAGS += $(INCLUDE_DIR)

# 递归找所有 .cpp
SRCS := $(shell find $(SRC_DIRS) -type f -name '*.cpp')
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# 输出文件
TARGET = $(BIN_DIR)/NovaSmtp.out

# Phony targets
.PHONY: all clean run info dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

info:
	@echo "CXX         = $(CXX)"
	@echo "INCLUDE_DIR = $(INCLUDE_DIR)"
	@echo "TARGET      = $(TARGET)"
	@echo "SRCS        = $(words $(SRCS)) files"
	@echo "CXXFLAGS    = $(CXXFLAGS)"


# 链接
$(TARGET): $(OBJS) $(LIB)
	@mkdir -p $(dir $@)
	@$(CXX) $(OBJS) -o $@ $(LDLIBS)

# 将 ./src/a/b.cpp -> ./obj/./src/a/b.o
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# 自动依赖
-include $(DEPS)

run: all
	@sudo $(TARGET)

clean:
	@rm -vrf $(OBJ_DIR) $(BIN_DIR)