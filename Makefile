# Compiler dan flags
CC = gcc
CFLAGS = -Wall -Iinclude

# === Direktori ===
SRC_DIR = source/berry
BUILD_DIR = build
INCLUDE_DIR = include/berry
TEST_DIR = test
EXEC_DIR = test_exec

# === MATRIX ===
MATRIX_SRC = $(SRC_DIR)/matrix/matrix_driver.c
MATRIX_OBJ = $(BUILD_DIR)/matrix_driver.o
MATRIX_TEST = $(TEST_DIR)/test_matrix.c
MATRIX_EXEC = $(EXEC_DIR)/matrix_exec

# === IRC ===
IRC_SRC = $(SRC_DIR)/irc/irc_driver.c
IRC_OBJ = $(BUILD_DIR)/irc_driver.o
IRC_TEST = $(TEST_DIR)/test_irc.c
IRC_EXEC = $(EXEC_DIR)/irc_exec

# === Direktori build dan exec ===
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXEC_DIR):
	mkdir -p $(EXEC_DIR)

# === Build objek matrix dan irc ===
$(MATRIX_OBJ): $(MATRIX_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(IRC_OBJ): $(IRC_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# === Build executable untuk matrix test ===
$(MATRIX_EXEC): $(MATRIX_TEST) $(MATRIX_OBJ) | $(EXEC_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# === Build executable untuk irc test ===
$(IRC_EXEC): $(IRC_TEST) $(IRC_OBJ) | $(EXEC_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# === Target utama ===
all: $(MATRIX_EXEC) $(IRC_EXEC)

# === Clean ===
clean:
	rm -rf $(BUILD_DIR) $(EXEC_DIR)

.PHONY: all clean
