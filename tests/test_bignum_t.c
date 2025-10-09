#include "bignum.h"
#include <assert.h>
#include <stdio.h>

void test_struct_size() {
    assert(sizeof(bignum_t) > 0);
    printf("Test passed: sizeof(bignum_t) is valid.\n");
}

int main() {
    test_struct_size();
    return 0;
}