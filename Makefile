cc := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
CFLAGS_DEBUG := -std=c11 -Wall -Wextra -Wpedantic -g -O0

SRC_DIR := src 
BUILD_DIR := build 
BIN_DIR := bin

TARGET := $(BIN_DIR)/mos6502

SRC := $(shell find  $(SRC_DIR) -name '*.c')
OBJ := $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(cc) $(OBJ) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(cc) $(CFLAGS) -c $< -o $@

debug: CFLAGS=$(CFLAGS_DEBUG)
debug: clean $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all
.PHONY: all clean rebuild debug