CYCLE_PER_US = 1800L

ifndef CYCLE_PER_US
$(error CYCLE_PER_US not set. Set CYCLE_PER_US according to your machine CPU-SPEED)
endif

CC = gcc
CFLAGS = -Wall -g -DCYCLE_PER_US=$(CYCLE_PER_US)

# Include directory where fiber.h is located
INCLUDE_DIR = /home/souparna/diss/sched-sync/libfiber/include

# Directory where the libfiber library is located
LIB_DIR = /home/souparna/diss/sched-sync/libfiber

export LD_LIBRARY_PATH +=$(LIB_DIR)

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR_LOCAL = ./include

# Source files and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/subversion_demo

# Add -pthread flag for threading support and include directory
CFLAGS += -I$(INCLUDE_DIR) -I$(INCLUDE_DIR_LOCAL) -pthread

# Library flags
LDFLAGS = -L$(LIB_DIR) -lfiber

# Rule for the target executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule for creating object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Create the binary directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean up object files, binary files, and directories
clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

# Target for building with FAIRLOCK
fairlock: CFLAGS += -DFAIRLOCK
fairlock: $(TARGET)

# Target for building with MUTEX
mutex: CFLAGS += -DMUTEX
mutex: $(TARGET)

#Target for building with SCHEDLOCK
schedlock: CFLAGS += -DSCHEDLOCK
schedlock: $(TARGET)


.PHONY: clean fairlock mutex
