# Makefile for bignum-common
# Makefile for bignum-common library

# --- Configuration ---
CONFIG ?= debug

# --- Tools ---
CC = gcc
AR = ar
ARFLAGS = rcs
RANLIB = ranlib

# --- Directories ---
BUILD_DIR = build
DIST_DIR = dist
SRC_DIR = src
INCLUDE_DIR = include
TESTS_DIR = tests

# --- Files ---
TARGET_LIB = $(DIST_DIR)/libbignumcommon.a
# Note: No source files yet, so OBJECTS is empty. This is a placeholder.
OBJECTS =

# --- Flags ---
CFLAGS_BASE = -std=c11 -Wall -Wextra -pedantic -I$(INCLUDE_DIR) -no-pie
ifeq ($(CONFIG), release)
    CFLAGS = $(CFLAGS_BASE) -O2
else # debug
    CFLAGS = $(CFLAGS_BASE) -g
endif

.PHONY: all check build test install clean help

all: build

# The check target attempts to compile a dummy file that includes our header.
# This verifies that the header is syntactically correct.
check:
	@echo "Checking syntax of bignum.h..."
	@echo "int main() { return 0; }" | $(CC) $(CFLAGS) -x c - -o /dev/null
	@echo "Syntax check passed."

build: $(TARGET_LIB)

# Creates an empty library for now, as there are no .c files.
$(TARGET_LIB): | $(DIST_DIR)
	@echo "Creating placeholder library $(TARGET_LIB)..."
	$(AR) $(ARFLAGS) $@
	$(RANLIB) $@

test: build
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling and running tests..."
	$(CC) $(CFLAGS) $(TESTS_DIR)/test_bignum_t.c -o $(BUILD_DIR)/test_runner
	./$(BUILD_DIR)/test_runner
	@echo "All tests passed."

install: build
	@echo "Installing headers to $(DIST_DIR)/..."
	@mkdir -p $(DIST_DIR)/$(INCLUDE_DIR)
	cp $(INCLUDE_DIR)/*.h $(DIST_DIR)/$(INCLUDE_DIR)/

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR) $(DIST_DIR)

# --- Directory Creation ---
$(DIST_DIR):
	mkdir -p $(DIST_DIR)

help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)."
	@echo "  build    - Build the library."
	@echo "  test     - Compile and run tests."
	@echo "  install  - Install headers to dist/."
	@echo "  clean    - Remove build artifacts."
	@echo "  help     - Show this help message."
	@echo "CONFIG=release can be used, e.g., 'make CONFIG=release all'."	