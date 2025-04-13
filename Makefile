CC = gcc
CFLAGS = -Wall -I./include -I./include/berry/ircmatrix
LIBS = -lcurl -ljson-c -lpthread  # Menambahkan library json-c untuk parsing JSON

# Direktori sumber
SRC_DIR = source/berry/ircmatrix
INCLUDE_DIR = include/berry/ircmatrix
TEST_DIR = test

# Menyusun daftar file header (.h) dan file sumber (.c)
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

# Menentukan nama file output
TARGET_LIB = libchat.a
TEST_EXEC = test_exec  # Nama eksekusi file tes

# Aturan utama
all: $(TARGET_LIB) $(TEST_EXEC)

# Membuat library statis
$(TARGET_LIB): $(OBJECTS)
		ar rcs $@ $^

# Membuat file eksekusi tes
$(TEST_EXEC): $(TEST_DIR)/test.c $(TARGET_LIB)
		$(CC) $(CFLAGS) -o $(TEST_EXEC) $(TEST_DIR)/test.c $(OBJECTS) $(LIBS)

# Aturan untuk membuat file objek dari file sumber
%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

# Membersihkan file yang dihasilkan
clean:
		rm -f $(OBJECTS) $(TARGET_LIB) $(TEST_EXEC)