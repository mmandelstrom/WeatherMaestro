# =================================================
# ======= 🌤WeatherMaestro Server Makefile ========
# =================================================

# Compiler (use clang on macOS, gcc otherwise)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CC := clang
else
	CC := gcc
endif

BUILD_DIR := build

# Source files directories
SRC_DIR   := src

# libs and utils source files to build
UTILS_SRC := ../utils/src/utils.c ../utils/src/linked_list.c
LIBS_SRC := ../libs/src/HTTPStatusCodes.c

ALL_SRC := $(SRC) $(UTILS_SRC) $(LIBS_SRC)

# Temp dirs to be deleted on clean
CACHE_DIR := data/cache
CITIES_DIR := data/cities

# Compilation flags
# CFLAGS := -std=c99 -MMD -MP -Wall -Wextra -Werror -Wfatal-errors -Wno-format-truncation -I../libs
CFLAGS := -std=c99 -MMD -MP -g -I../libs -I../utils

# Linker flags (auto-detect platform)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -flto
else
	LDFLAGS := -flto -Wl,--gc-sections
endif

# Linked libraries
# LIBS := -lcurl

# Find all .c source files recursively
SRC := $(shell find -L $(SRC_DIR) -type f -name '*.c')

# Map .c → .o in the build directory, combining module-specific, libs and utils files
UTILS_OBJ := $(patsubst ../utils/src/%.c,$(BUILD_DIR)/utils/%.o,$(UTILS_SRC))
LIBS_OBJ := $(patsubst ../libs/src/%.c,$(BUILD_DIR)/libs/%.o,$(LIBS_SRC))
SRC_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
OBJ := $(SRC_OBJ) $(UTILS_OBJ) $(LIBS_OBJ)

# Dependency files (.d)
DEP := $(OBJ:.o=.d)

# Binary name
BIN := ../server_app

# ==================== Recipes ====================

all: $(BIN)
	@echo "Build complete."

$(BIN): $(OBJ)
	@echo "Linking..."
	@$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

$(BUILD_DIR)/utils/%.o: ../utils/src/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libs/%.o: ../libs/src/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	@echo "Running $<..."
	@./$(BIN)

clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(CACHE_DIR) $(CITIES_DIR) $(BIN)
	@echo "Clean complete."

valgrind: $(BIN)
	@echo "Debugging $< using valgrind..."
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BIN)

gdb: $(BIN)
	@echo "Debugging $< using gdb..."
	@gdb ./$(BIN)

print:
	@echo "Source files: $(SRC)"
	@echo "Object files: $(OBJ)"
	@echo "Dependency files: $(DEP)"

-include $(DEP)

# ==================== Phonies ====================

.PHONY: all run clean print gdb valgrind
