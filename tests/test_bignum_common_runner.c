/**
 * @file    test_bignum_common_runner.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    18.06.2026
 *
 * @brief   Лёгкий smoke-раннер для ядра bignum (bignum_common.c).
 *
 * @details
 *   Компактный раннер на `assert.h` — проверяет, что ядро в принципе
 *   живо и основные функции не сломаны:
 *     - test_struct_size    — структура компилируется и имеет
 *                            ожидаемый sizeof;
 *     - test_bignum_init    — обнуление работает;
 *     - test_init_u64       — заполнение из uint64_t;
 *     - test_init_array     — копирование из массива + нормализация;
 *     - test_is_zero        — базовая проверка нулевого состояния;
 *     - test_clear_tail     — обнуление хвоста;
 *     - test_normalize      — нормализация.
 *
 *   Парсинг строк (hex/dec) намеренно НЕ тестируется здесь — он живёт
 *   в модуле `bignum_helpers` (см. tests/test_bignum_helpers.c).
 *
 * @note    Этот файл — smoke-раннер. Детальный coverage с граничными
 *          случаями и проверкой инвариантов живёт в
 *          tests/test_bignum_common.c. Оба держатся параллельно:
 *          раннер — для быстрого прогона, сьют — для глубокого
 *          покрытия.
 *
 *   Сборка:
 *     cc -std=c11 -Wall -Wextra -Wpedantic -O2 \
 *        bignum_common.c tests/test_bignum_common_runner.c \
 *        -I include -o tests/test_bignum_common_runner
 */

#include "bignum_common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Smoke: структура доступна и имеет ненулевой размер                 */
/* ------------------------------------------------------------------ */

void test_struct_size() {
    /* sizeof должен быть > 0 — компилятор уже отверг бы пустую структуру,
     * но проверяем явно, чтобы зафиксировать инвариант в раннере. */
    assert(sizeof(bignum_t) > 0);
    assert(sizeof(bignum_t) ==
           BIGNUM_CAPACITY * sizeof(uint64_t) + sizeof(size_t));
    printf("Test passed: sizeof(bignum_t) is valid.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_init: обнуляет len и весь массив words                     */
/* ------------------------------------------------------------------ */

void test_bignum_init() {
    bignum_t b;
    /* Заполняем «мусором», чтобы проверить, что init действительно всё
     * затирает, а не просто доверяет тому, что стек уже нулевой. */
    memset(&b, 0xAB, sizeof(b));

    bignum_init(&b);

    assert(b.len == 0);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == 0);
    }
    printf("Test passed: bignum_init() zeroes the structure.\n");

    /* NULL — тихо выходим, без падения. */
    bignum_init(NULL);
    printf("Test passed: bignum_init(NULL) is safe.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_init_u64: ноль, единица, UINT64_MAX                        */
/* ------------------------------------------------------------------ */

void test_init_u64() {
    bignum_t b;

    /* 0 → len == 0, массив нулевой */
    bignum_init_u64(&b, 0);
    assert(b.len == 0);
    assert(bignum_is_zero(&b));
    printf("Test passed: bignum_init_u64(0) → zero.\n");

    /* 1 → len == 1, words[0] == 1, хвост чистый */
    bignum_init_u64(&b, 1);
    assert(b.len == 1);
    assert(b.words[0] == 1);
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == 0);
    }
    printf("Test passed: bignum_init_u64(1) → len=1, words[0]=1.\n");

    /* UINT64_MAX → одно слово, всё биты */
    bignum_init_u64(&b, UINT64_MAX);
    assert(b.len == 1);
    assert(b.words[0] == UINT64_MAX);
    printf("Test passed: bignum_init_u64(UINT64_MAX) → single word.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_init_from_array: нормальный путь + нормализация хвоста     */
/* ------------------------------------------------------------------ */

void test_init_from_array() {
    bignum_t b;
    uint64_t src[3] = { 0xAA, 0xBB, 0xCC };
    int rc;

    rc = bignum_init_from_array(&b, src, 3);
    assert(rc == BIGNUM_SUCCESS);
    assert(b.len == 3);
    assert(b.words[0] == 0xAA);
    assert(b.words[1] == 0xBB);
    assert(b.words[2] == 0xCC);
    printf("Test passed: bignum_init_from_array() copies correctly.\n");

    /* Нормализация: старшие нули должны быть срезаны. */
    uint64_t src2[3] = { 0xAA, 0, 0 };
    rc = bignum_init_from_array(&b, src2, 3);
    assert(rc == BIGNUM_SUCCESS);
    assert(b.len == 1);
    assert(b.words[0] == 0xAA);
    printf("Test passed: bignum_init_from_array() normalizes trailing zeros.\n");

    /* NULL → NULL_ARG, без падения. */
    rc = bignum_init_from_array(NULL, src, 3);
    assert(rc == BIGNUM_ERROR_NULL_ARG);
    printf("Test passed: bignum_init_from_array(NULL, ...) → NULL_ARG.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_is_zero: NULL, ноль, ненулевое                             */
/* ------------------------------------------------------------------ */

void test_is_zero() {
    bignum_t b;

    /* NULL — считается нулём по контракту. */
    assert(bignum_is_zero(NULL) == 1);

    bignum_init(&b);
    assert(bignum_is_zero(&b) == 1);

    bignum_init_u64(&b, 1);
    assert(bignum_is_zero(&b) == 0);

    bignum_init_u64(&b, UINT64_MAX);
    assert(bignum_is_zero(&b) == 0);

    printf("Test passed: bignum_is_zero() behaves as specified.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_clear_tail: обнуляет только хвост                          */
/* ------------------------------------------------------------------ */

void test_clear_tail() {
    bignum_t b;
    bignum_init(&b);

    /* Заполним весь массив единицами, потом обнулим хвост с индекса 16. */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 1;

    bignum_clear_tail(&b, 16);

    for (size_t i = 0; i < 16; ++i) {
        assert(b.words[i] == 1);
    }
    for (size_t i = 16; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == 0);
    }
    printf("Test passed: bignum_clear_tail() zeroes only the tail.\n");

    /* start == CAPACITY — допустимый no-op. */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xFF;
    bignum_clear_tail(&b, BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == 0xFF);
    }
    printf("Test passed: bignum_clear_tail(CAPACITY) is a no-op.\n");
}

/* ------------------------------------------------------------------ */
/*  bignum_normalize: срезка нулей и очистка хвоста                   */
/* ------------------------------------------------------------------ */

void test_normalize() {
    bignum_t b;
    bignum_init(&b);

    /* Старшие нули в массиве: len выставлен заведомо большим, */
    /* после normalize должен ужаться до первого ненулевого.   */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0;
    b.words[0] = 0x42;
    b.len      = BIGNUM_CAPACITY;

    bignum_normalize(&b);

    assert(b.len == 1);
    assert(b.words[0] == 0x42);
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == 0);
    }
    printf("Test passed: bignum_normalize() trims trailing zeros.\n");

    /* Число на полную ёмкость — должно сохраниться без изменений. */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = UINT64_MAX;
    b.len = BIGNUM_CAPACITY;

    bignum_normalize(&b);

    assert(b.len == BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == UINT64_MAX);
    }
    printf("Test passed: bignum_normalize() keeps full-capacity values.\n");

    /* NULL — без падения. */
    bignum_normalize(NULL);
    printf("Test passed: bignum_normalize(NULL) is safe.\n");
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */

int main() {
    test_struct_size();
    test_bignum_init();
    test_init_u64();
    test_init_from_array();
    test_is_zero();
    test_clear_tail();
    test_normalize();
    return 0;
}