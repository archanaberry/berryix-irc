# Compiler dan flags
CC = gcc
CFLAGS = -Wall -Iinclude

# Direktori
SRC_DIR = source/berry/matrix
BUILD_DIR = build
INCLUDE_DIR = include
TEST_DIR = test
EXEC_DIR = test_exec

# File sumber
SRC = $(SRC_DIR)/matrix_driver.c
OBJ = $(BUILD_DIR)/matrix_driver.o
TEST = $(TEST_DIR)/test.c
TARGET = $(EXEC_DIR)/test_exec

# Buat direktori build dan exec jika belum ada
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXEC_DIR):
	mkdir -p $(EXEC_DIR)

# Build objek
$(OBJ): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build final executable
$(TARGET): $(TEST) $(OBJ) | $(EXEC_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Target utama
all: $(TARGET)

# Clean file objek dan hasil compile
clean:
	rm -rf $(BUILD_DIR) $(EXEC_DIR)

.PHONY: all clean