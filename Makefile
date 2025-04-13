# === Compiler dan flags ===
CC = gcc
CFLAGS = -Wall -Iinclude/berry -Iinclude/berry/matrix -Iinclude/berry/irc
LDFLAGS = -lcurl -ljson-c

# === Direktori ===
INCLUDE_DIR = include/berry
SOURCE_DIR = source/berry
MATRIX_DIR = matrix
IRC_DIR = irc
TEST_DIR = test
BIN_DIR = bin
OBJ_DIR = build

# === File sumber utama ===
MATRIX_SRC = $(SOURCE_DIR)/$(MATRIX_DIR)/matrix_driver.c
IRC_SRC = $(SOURCE_DIR)/$(IRC_DIR)/irc_driver.c

# === File header ===
MATRIX_HEADER = $(INCLUDE_DIR)/$(MATRIX_DIR)/matrix_driver.h
IRC_HEADER = $(INCLUDE_DIR)/$(IRC_DIR)/irc_driver.h

# === File test ===
MATRIX_TEST = $(TEST_DIR)/test_matrix.c
IRC_TEST = $(TEST_DIR)/test_irc.c

# === Output eksekusi ===
MATRIX_EXEC = $(BIN_DIR)/test_matrix
IRC_EXEC = $(BIN_DIR)/test_irc

.PHONY: all clean test-matrix test-irc run

# === Target utama ===
all: $(MATRIX_EXEC) $(IRC_EXEC)

# === Buat direktori bin kalau belum ada ===
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# === Build test_matrix ===
$(MATRIX_EXEC): $(MATRIX_TEST) $(MATRIX_SRC) $(MATRIX_HEADER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(MATRIX_TEST) $(MATRIX_SRC) -o $@ $(LDFLAGS)

# === Build test_irc ===
$(IRC_EXEC): $(IRC_TEST) $(IRC_SRC) $(IRC_HEADER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(IRC_TEST) $(IRC_SRC) -o $@ $(LDFLAGS)

# === Bersihkan hasil build ===
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

# === Jalankan masing-masing test ===
test-matrix: $(MATRIX_EXEC)
	./$(MATRIX_EXEC)

test-irc: $(IRC_EXEC)
	./$(IRC_EXEC)

# === Default run ===
run: test-matrix
