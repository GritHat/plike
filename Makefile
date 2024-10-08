# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c17

# Libraries to link (add libraries here)
LIBS = -lm

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Output executable name
TARGET = $(BIN_DIR)/plike2C

# Default rule
all: $(TARGET)

# Rule to link the object files and create the executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Rule to compile .c files into .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Clean up object files and the executable
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Add future source files here (optional)
# Future files should be placed in src/ and compiled automatically
# Add libraries in the LIBS field for linking
# Example: LIBS = -lm -lnosys (for math and nosys libraries)

.PHONY: all clean