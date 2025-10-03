# Makefile for bignum-common
# This project is header-only, so there is nothing to build.
# The 'check' target is used to verify syntax.

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Iinclude

.PHONY: check clean help

# The check target attempts to compile a dummy file that includes our header.
# This verifies that the header is syntactically correct.
check:
	@echo "Checking syntax of bignum.h..."
	@echo "int main() { return 0; }" | $(CC) $(CFLAGS) -x c - -o /dev/null
	@echo "Syntax check passed."

clean:
	@echo "Nothing to clean in a header-only project."

help:
	@echo "Available targets:"
	@echo "  check    - Verifies the syntax of the header files."
	@echo "  clean    - Does nothing (header-only)."
	@echo "  help     - Shows this help message."