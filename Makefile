# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source file
SRC = $(SRC_DIR)/ls-v1.0.0.c

# Object file
OBJ = $(OBJ_DIR)/ls-v1.0.0.o

# Executable name
TARGET = $(BIN_DIR)/ls

# Default target
all: $(TARGET)

# Rule to build executable
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to build object file
$(OBJ): $(SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Clean build files
clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/ls

# Phony targets
.PHONY: all clean
