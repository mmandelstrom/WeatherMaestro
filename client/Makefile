# =================================================
# ======= 🌤WeatherMaestro Server Makefile ========
# =================================================

BUILD_DIR := build

# Source files directories
SRC_DIR   		:= src
CORE_SRC_DIR  := core/modules/src
UTILS_SRC_DIR := core/utils/src
LIBS_SRC_DIR  := libs/src

# Source files to build
SRC       := $(shell find -L $(SRC_DIR) 	-type f -name '*.c')
CORE_SRC  := $(shell find -L ../$(CORE_SRC_DIR)  -type f -name '*.c')
UTILS_SRC := $(shell find -L ../$(UTILS_SRC_DIR) -type f -name '*.c')
LIBS_SRC  := ../libs/src/HTTPStatusCodes.c ../libs/src/cJSON.c

ALL_SRC := $(SRC) $(CORE_SRC) $(UTILS_SRC) $(LIBS_SRC)

# Find all .c source files recursively

# Temp dirs to be deleted on clean
CACHE_DIR := data/cache

# Compilation flags
# CFLAGS := -std=c99 -MMD -MP -Wall -Wextra -Werror -Wfatal-errors -Wno-format-truncation -I../libs
# CFLAGS := -std=c99 -MMD -MP -g -I../libs -I../utils
CFLAGS := -std=c89 -MMD -MP -Wall -Wextra -Wfatal-errors -g -Iinclude -I../libs/include -I../core/utils/include -I../core/modules/include

# Linked libraries
LIBS := -lcurl

# Map .c → .o
SRC_OBJ   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/$(SRC_DIR)/%.o,$(SRC))
CORE_OBJ  := $(patsubst ../$(CORE_SRC_DIR)/%.c,$(BUILD_DIR)/$(CORE_SRC_DIR)/%.o,$(CORE_SRC))
UTILS_OBJ := $(patsubst ../$(UTILS_SRC_DIR)/%.c,$(BUILD_DIR)/$(UTILS_SRC_DIR)/%.o,$(UTILS_SRC))
LIBS_OBJ  := $(patsubst ../$(LIBS_SRC_DIR)/%.c,$(BUILD_DIR)/$(LIBS_SRC_DIR)/%.o,$(LIBS_SRC))

OBJ := $(SRC_OBJ) $(CORE_OBJ) $(UTILS_OBJ) $(LIBS_OBJ)

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

$(BUILD_DIR)/$(CORE_SRC_DIR)/%.o: ../$(CORE_SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(UTILS_SRC_DIR)/%.o: ../$(UTILS_SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(LIBS_SRC_DIR)/%.o: ../$(LIBS_SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	@echo "Running $<..."
	@./$(BIN)

clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(BIN)
	@echo "Clean complete."

full-clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(CACHE_DIR) $(BIN)
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
