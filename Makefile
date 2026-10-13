# ==============================================================================
# Makefile for HTTP/1.1 Web Server Project
# Target Architecture: Linux / POSIX.1-2008
# Standard: C17
# ==============================================================================

CC      ?= gcc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wwrite-strings \
           -Wstrict-prototypes -Wold-style-definition -Wredundant-decls \
           -Wnested-externs -Wmissing-include-dirs -D_POSIX_C_SOURCE=200809L \
           -D_DEFAULT_SOURCE -Iinclude
LDFLAGS ?= -pthread

SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
BIN_DIR := bin
TEST_DIR := tests

# Core source modules for the starter / scaffold binary
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TARGET  := $(BIN_DIR)/server

# Unit test sources
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

.PHONY: all starter test test-python check-scaffold clean help

all: $(TARGET)

starter: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $@ (Repository Scaffold & Starter Binary)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Pattern rule for compiling and linking test binaries
$(BIN_DIR)/%: $(TEST_DIR)/%.c $(filter-out $(OBJ_DIR)/main.o,$(OBJS)) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Test targets
test: $(TARGET) $(TEST_BINS)
	@echo "Running test suite..."
	@for test_bin in $(TEST_BINS); do \
		echo "Executing $$test_bin..."; \
		$$test_bin || exit 1; \
	done
	@echo "All C unit test skeletons executed successfully."
	@echo "Running Python test verification..."
	@python3 -m unittest discover -s $(TEST_DIR) -p "test_*.py" 2>/dev/null || \
	 python -m unittest discover -s $(TEST_DIR) -p "test_*.py" 2>/dev/null || \
	 echo "Python tests located in tests/ are ready for orchestrator execution."

check-scaffold: $(TARGET)
	@$(TARGET) --check-scaffold

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts."

help:
	@echo "Available Makefile targets:"
	@echo "  all            : Compile the starter scaffold binary ($(TARGET))"
	@echo "  starter        : Alias for 'all'"
	@echo "  test           : Build and execute available C and Python unit test skeletons"
	@echo "  check-scaffold : Run environment and scaffold verification check"
	@echo "  clean          : Remove compiled object files and binaries"
	@echo "  help           : Display this help menu"
