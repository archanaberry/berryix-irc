# Compiler dan flags
CC = gcc
CFLAGS = -Wall -I../include/berry/matrix
LDFLAGS = -lcurl -ljson-c

# Direktori
INCLUDE_DIR = ../include/berry
SOURCE_DIR = ../source/berry
MATRIX_DIR = matrix
OBJ_DIR = ../build
BIN_DIR = ./

# File sumber dan objek
SRC = $(SOURCE_DIR)/$(MATRIX_DIR)/matrix_driver.c
OBJ = $(OBJ_DIR)/matrix_driver.o

# File uji
TEST = test.c
TARGET = $(BIN_DIR)/test_exec

.PHONY: all clean run

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ): $(SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ) $(TEST) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(TEST) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: all
	./$(TARGET)