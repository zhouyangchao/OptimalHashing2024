# OptimalHashing Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -I./include
LDFLAGS = -lm -lpthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = obj
LIB_DIR = lib
BENCHMARK_DIR = benchmark

# Source files
COMMON_SRC = $(SRC_DIR)/common.c
LIB_SRC = $(SRC_DIR)/elastic_hash.c $(SRC_DIR)/funnel_hash.c $(SRC_DIR)/linear_hash.c $(SRC_DIR)/uniform_hash.c $(SRC_DIR)/hash_ops.c
BENCHMARK_SRC = $(BENCHMARK_DIR)/benchmark.c

# Object files
COMMON_OBJ = $(OBJ_DIR)/common.o
LIB_OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRC))
BENCHMARK_OBJ = $(OBJ_DIR)/benchmark.o

# Binaries
LIB = $(LIB_DIR)/liboptimalhash.a
BENCHMARK = $(BIN_DIR)/benchmark

# Default target
all: prepare lib benchmark

# Prepare directories
prepare:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR)

# Compile common code
$(COMMON_OBJ): $(COMMON_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile library source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/%.h $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/hash_ops.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build the static library
lib: $(COMMON_OBJ) $(LIB_OBJ)
	ar rcs $(LIB) $^

# Compile benchmark
$(BENCHMARK_OBJ): $(BENCHMARK_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Build benchmark executable
benchmark: $(BENCHMARK_OBJ) $(LIB)
	$(CC) $(BENCHMARK_OBJ) -o $(BENCHMARK) -L$(LIB_DIR) -loptimalhash $(LDFLAGS)

# Clean build files
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* $(LIB_DIR)/*

# Install (optional)
install: all
	@mkdir -p $(DESTDIR)/usr/local/bin
	@mkdir -p $(DESTDIR)/usr/local/lib
	install -m 755 $(BENCHMARK) $(DESTDIR)/usr/local/bin/
	install -m 644 $(LIB) $(DESTDIR)/usr/local/lib/

.PHONY: all prepare lib benchmark clean install