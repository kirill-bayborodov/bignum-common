# bignum-common

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-common/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-common/actions/workflows/ci.yml)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-common?label=release)](https://github.com/kirill-bayborodov/bignum-common/releases/latest)

`bignum-common` is a foundational module for the `bignum-lib` project. It provides the core data structures, type definitions, and basic initialization/normalization functions used by all other bignum modules.

## Distribution

Part of the `bignum-lib` project: https://github.com/kirill-bayborodov/bignum-lib
Also available as a standalone distribution.

## Features

-   **Core Data Structures:** Defines the primary `bignum_t` structure for arbitrary-precision integers (up to 2048 bits capacity to cover all edge cases for `double` formatting).
-   **Standardized Error Handling:** Provides `bignum_status_t` codes for consistent error management across the library.
-   **High Performance:** Optimized x86-64 assembly and C implementations for core operations.
-   **Dependency-Free Core:** The core logic has no external runtime dependencies.
-   **Automated Builds:** A comprehensive `Makefile` for easy compilation, testing, and benchmarking.
-   **Continuous Integration:** All changes are automatically built and tested via GitHub Actions.
-   **Static Analysis:** Code quality is enforced using `cppcheck` for all C-based test files.

## Dependencies

-   **Build-time:** `make`, `gcc`, `yasm`, `cppcheck`.
-   **Component:** This is the base module and has no external component dependencies.

To clone the repository, use:
```bash
git clone https://github.com/kirill-bayborodov/bignum-common.git
```

## API

The library provides core structures and functions, declared in `include/bignum.h` and `include/bignum_common.h`.

### Data Structures

```c
typedef enum {
    BIGNUM_SUCCESS         =  0,
    BIGNUM_ERROR_NULL_ARG  = -1,
    BIGNUM_ERROR_OVERFLOW  = -2
} bignum_status_t;

typedef struct {
    uint64_t words[32]; // 32 * 64 = 2048 bits capacity
    size_t   len;       // Number of used words
} bignum_t;
```

### Core Functions

```c
// Zeros out a bignum_t (len = 0, words = 0).
void bignum_init(bignum_t *x);

// Initializes a bignum_t from a 64-bit integer.
void bignum_init_u64(bignum_t *b, uint64_t val);

// Initializes a bignum_t from an array of uint64_t (least significant word first).
// Returns BIGNUM_SUCCESS, BIGNUM_ERROR_NULL_ARG, or BIGNUM_ERROR_OVERFLOW.
int bignum_init_from_array(bignum_t *dst, const uint64_t *src, size_t src_len);

// Returns 1 if x == NULL or x->len == 0, otherwise 0.
int bignum_is_zero(const bignum_t *x);

// Zeros out the "tail" of the words array from the given start index to the end of capacity.
void bignum_clear_tail(bignum_t *x, size_t start);

// Removes leading zero words and clears the tail. 
// Must be called after any arithmetic operation to guarantee no uninitialized data.
void bignum_normalize(bignum_t *x);
```

## How to Build, Test, Install and Use

The project uses a `Makefile` to manage all tasks.

### Build the code
Builds the object file.
```bash
make build CONFIG=release
```

### Run Unit Tests
Compiles and runs fast, essential correctness tests.
```bash
make test CONFIG=release
```

### Run Static Analysis
Checks all C source files (`tests/`, `benchmarks/` and `dist/`) for potential bugs and style issues.
```bash
make lint
```

### Run Performance Benchmarks
Compiles and runs performance tests using `perf`. The txt report is saved to `benchmarks/reports/check_*.txt`.
```bash
make bench CONFIG=debug
```

### Build the installation pack distributive
Builds the installation pack of files (with objects .o file) in dist directory.
```bash
make install CONFIG=release
```

### Build the single-header distributive pack
Builds the distributive pack of files (with single-header and static library .a file).
```bash
make dist CONFIG=release
```

## Clean Up

To remove all generated files (object files, executables, reports):
```bash
make clean
```

## How to Use

This project produces an object file (`bignum_common.o`) which you can link with your own application.

**1. Clone the repository:**
```bash
git clone https://github.com/kirill-bayborodov/bignum-common.git
cd bignum-common
```

**2. Build the object file:**
```bash
make build
```
The output will be located at `build/bignum_common.o`.

**3. Link with your application:**
When compiling your project, include the object file and specify the include paths for the headers.
```bash
gcc your_app.c build/bignum_common.o -I./include -o your_app -no-pie
```

## Contributing

Contributions are welcome! Please follow these steps:
1.  Fork the repository.
2.  Create a new branch (`git checkout -b feature/AmazingFeature`).
3.  Make your changes.
4.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
5.  Push to the branch (`git push origin feature/AmazingFeature`).
6.  Open a Pull Request.

When creating Issues or Pull Requests, please use the provided templates to ensure all necessary information is included.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.