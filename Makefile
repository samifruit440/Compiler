# Makefile for the incremental compiler
# Supports both Runtime Evaluation (RTE) and Compile-Time Evaluation (CTE) modes

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
AS = as
LD = ld

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build
SCRIPTS_DIR = scripts

# Source files
COMPILER_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/codegen.c $(SRC_DIR)/ast.c
TEST_SRC = $(TEST_DIR)/compiler_test.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/codegen.c $(SRC_DIR)/ast.c

# Output binaries (all in build directory)
COMPILER_RTE = build/compiler-rte
COMPILER_CTE = build/compiler-cte
TEST_RUNNER = build/test_runner
TEST_RUNNER_CTE = build/test_runner_cte

.PHONY: all clean test build build-rte build-cte build-all test-rte test-cte test-all help

# Default target: build standard RTE compiler
all: build

# Standard build (RTE mode only)
build: $(COMPILER_RTE)

# Build RTE compiler (Runtime Evaluation)
build-rte: $(COMPILER_RTE)

$(COMPILER_RTE): $(COMPILER_SRCS) $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/codegen.h $(SRC_DIR)/tags.h $(SRC_DIR)/ast.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $(COMPILER_RTE) $(COMPILER_SRCS)
	@echo "✓ Compiler built: $(COMPILER_RTE)"

# Build CTE compiler (Compile-Time Evaluation)
build-cte: $(COMPILER_CTE)

$(COMPILER_CTE): $(COMPILER_SRCS) $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/codegen.h $(SRC_DIR)/tags.h $(SRC_DIR)/ast.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $(COMPILER_CTE) $(COMPILER_SRCS)
	@echo "✓ CTE compiler built: $(COMPILER_CTE)"

# Build both compilers
build-all: build-rte build-cte
	@echo "✓ Both compilers built"

# Build test runners
$(TEST_RUNNER): $(TEST_SRC) $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/codegen.h $(SRC_DIR)/tags.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -DTEST_MODE=0 -o $(TEST_RUNNER) $(TEST_SRC)
	@echo "✓ RTE test runner built"

$(TEST_RUNNER_CTE): $(TEST_SRC) $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/codegen.h $(SRC_DIR)/tags.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) -DTEST_MODE=1 -o $(TEST_RUNNER_CTE) $(TEST_SRC)
	@echo "✓ CTE test runner built"

# Run tests with RTE compiler
test-rte: $(COMPILER_RTE) $(TEST_RUNNER)
	@mkdir -p test_output
	@echo "=========================================="
	@echo "Running RTE tests (Runtime Evaluation)..."
	@echo "=========================================="
	@./$(TEST_RUNNER)

# Run tests with CTE compiler
test-cte: $(COMPILER_CTE) $(TEST_RUNNER_CTE)
	@mkdir -p test_output
	@echo "=========================================="
	@echo "Running CTE tests (Compile-Time Eval)..."
	@echo "=========================================="
	@./$(TEST_RUNNER_CTE)

# Run both test suites and compare
test-all: build-all $(TEST_RUNNER) $(TEST_RUNNER_CTE)
	@mkdir -p test_output $(SCRIPTS_DIR)
	@echo "=========================================="
	@echo "Running ALL tests (RTE + CTE + Compare)..."
	@echo "=========================================="
	@$(TEST_RUNNER)
	@echo ""
	@$(TEST_RUNNER_CTE)
	@echo ""
	@bash $(SCRIPTS_DIR)/compare_modes.sh

# Default test target (RTE)
test: test-rte

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -rf test_output/* out/*
	@echo "✓ Cleaned build artifacts"

# Help target
help:
	@echo "=========================================="
	@echo "Compiler Build Targets:"
	@echo "=========================================="
	@echo "  make build        - Build default RTE compiler"
	@echo "  make build-rte    - Build Runtime Evaluation compiler"
	@echo "  make build-cte    - Build Compile-Time Evaluation compiler"
	@echo "  make build-all    - Build both RTE and CTE compilers"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test         - Run tests with RTE compiler (default)"
	@echo "  make test-rte     - Run tests with RTE compiler"
	@echo "  make test-cte     - Run tests with CTE compiler"
	@echo "  make test-all     - Run both test suites and compare"
	@echo ""
	@echo "Utility:"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Evaluation Modes:"
	@echo "  RTE - Runtime Evaluation: generates assembly for operations"
	@echo "  CTE - Compile-Time Evaluation: pre-computes constant expressions"
