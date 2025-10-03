# bignum-common

**Version: 1.0.2**

`bignum-common` is a foundational module for the `bignum-lib` project. It provides the core data structures and type definitions used by all other bignum modules.

## Features

*   Defines the primary `bignum_t` structure for arbitrary-precision integers.
*   Provides standardized status codes (`bignum_status_t`) for error handling.
*   Designed for a 64-bit architecture.

## Usage

This module is intended to be used as a dependency (e.g., a Git submodule) in other projects. To use it, include the main header file:

```c
#include <bignum.h>

void my_function() {
    bignum_t my_number;
    // ...
}
```

You must add the `include` directory of this module to your compiler's include path. For example, using GCC:

```bash
gcc -I/path/to/bignum-common/include my_project.c
```

## Contributing

This is a module within the `bignum-lib` ecosystem. Please refer to the main project for contribution guidelines.
```
