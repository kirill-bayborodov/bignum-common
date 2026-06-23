/**
 * @file    test_bignum_common.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    17.06.2026
 *
 * @brief   Юнит-тесты ядра bignum (bignum_common.c).
 *
 * @details Тестируются:
 *   - bignum_init         (NULL-устойчивость, обнуление всех полей)
 *   - bignum_init_u64     (0, 1, MAX, NULL)
 *   - bignum_init_from_array (нормализация хвоста, NULL, переполнение по len,
 *                             len == 0)
 *   - bignum_is_zero      (NULL, ноль, ненулевое, "ложный ноль" со старшим = 0)
 *   - bignum_clear_tail   (все границы: 0, mid, CAPACITY, > CAPACITY, NULL)
 *   - bignum_normalize    (NULL, пустое, "грязные" хвосты, повторные вызовы,
 *                         инвариант words[len-1] != 0 для len > 0)
 *
 *   Парсинг строк (hex/dec) намеренно НЕ тестируется здесь —
 *   он живёт в модуле bignum-helpers
 */

#include "bignum_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0;

#define ASSERT(cond, msg)                                              \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL [%s:%d] %s\n",                       \
                    __FILE__, __LINE__, (msg));                        \
            ++fails;                                                   \
        }                                                              \
    } while (0)

#define OK(what)        printf("ok   [%s]\n", (what))

/* ------------------------------------------------------------------ */
/*  bignum_init                                                       */
/* ------------------------------------------------------------------ */

static void test_init(void)
{
    bignum_t b;

    /* Нулевая инициализация стека без явной очистки → проверяем,
     * что init действительно всё обнуляет. */
    memset(&b, 0xAB, sizeof(b));
    bignum_init(&b);
    ASSERT(b.len == 0, "init: len == 0");
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "init: words[i] == 0");
    }
    OK("init — все поля обнулены");

    /* NULL-устойчивость */
    bignum_init(NULL);
    OK("init(NULL) — без падения");
}

/* ------------------------------------------------------------------ */
/*  bignum_init_u64                                                   */
/* ------------------------------------------------------------------ */

static void test_init_u64(void)
{
    bignum_t b;

    /* 0: по контракту len == 0, words все нулевые */
    bignum_init_u64(&b, 0);
    ASSERT(b.len == 0, "u64(0): len == 0");
    ASSERT(bignum_is_zero(&b), "u64(0): is_zero");
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "u64(0): words[i] == 0");
    }
    OK("u64(0) — корректный ноль");

    /* 1: однозначное число */
    bignum_init_u64(&b, 1);
    ASSERT(b.len == 1 && b.words[0] == 1, "u64(1)");
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "u64(1): старшие слова нулевые");
    }
    OK("u64(1) — len=1, words[0]=1, хвост чистый");

    /* UINT64_MAX */
    bignum_init_u64(&b, UINT64_MAX);
    ASSERT(b.len == 1 && b.words[0] == UINT64_MAX, "u64(UINT64_MAX)");
    OK("u64(UINT64_MAX)");

    /* NULL — тихо выходим */
    bignum_init_u64(NULL, 42);
    OK("u64(NULL, ...) — без падения");

    /* Повторный вызов поверх ненулевого значения — должен затереть */
    bignum_init_u64(&b, UINT64_MAX);
    bignum_init_u64(&b, 7);
    ASSERT(b.len == 1 && b.words[0] == 7, "u64 поверх MAX");
    OK("u64 поверх не-нулевого — корректная перезапись");
}

/* ------------------------------------------------------------------ */
/*  bignum_init_from_array                                            */
/* ------------------------------------------------------------------ */

static void test_init_from_array(void)
{
    bignum_t b;
    uint64_t src[3] = { 0xAA, 0xBB, 0xCC };
    int rc;

    /* Нормальный путь */
    rc = bignum_init_from_array(&b, src, 3);
    ASSERT(rc == BIGNUM_SUCCESS, "array: rc == SUCCESS");
    ASSERT(b.len == 3, "array: len == 3");
    ASSERT(b.words[0] == 0xAA && b.words[1] == 0xBB && b.words[2] == 0xCC,
           "array: содержимое");
    OK("init_from_array — нормальный путь");

    /* Нормализация хвоста: если в src старшие слова нули — len ужмётся */
    uint64_t src2[3] = { 0xAA, 0, 0 };
    rc = bignum_init_from_array(&b, src2, 3);
    ASSERT(rc == BIGNUM_SUCCESS && b.len == 1 && b.words[0] == 0xAA,
           "array: нормализация хвоста");
    OK("init_from_array — нормализация хвоста");

    /* Полностью нулевой массив → len == 0 */
    uint64_t zeros[5] = { 0 };
    rc = bignum_init_from_array(&b, zeros, 5);
    ASSERT(rc == BIGNUM_SUCCESS && b.len == 0,
           "array: полностью нулевой → len=0");
    OK("init_from_array — полностью нулевой");

    /* len == 0, src != NULL — допустимо, эквивалентно init */
    uint64_t dummy[1] = { 0 };
    rc = bignum_init_from_array(&b, dummy, 0);
    ASSERT(rc == BIGNUM_SUCCESS && b.len == 0, "array: len=0, src!=NULL");
    OK("init_from_array — пустой массив (len=0)");

    /* src == NULL даже при len=0 → NULL_ARG (защита от чтения NULL) */
    rc = bignum_init_from_array(&b, NULL, 0);
    ASSERT(rc == BIGNUM_ERROR_NULL_ARG, "array: src==NULL, len=0 → NULL_ARG");
    OK("init_from_array — NULL_ARG на src (даже при len=0)");

    /* src == NULL с len > 0 → NULL_ARG */
    rc = bignum_init_from_array(&b, NULL, 1);
    ASSERT(rc == BIGNUM_ERROR_NULL_ARG, "array: src==NULL → NULL_ARG");
    OK("init_from_array — NULL_ARG на src");

    /* dst == NULL → NULL_ARG */
    rc = bignum_init_from_array(NULL, src, 3);
    ASSERT(rc == BIGNUM_ERROR_NULL_ARG, "array: dst==NULL → NULL_ARG");
    OK("init_from_array — NULL_ARG на dst");

    /* Переполнение по длине */
    uint64_t *big = (uint64_t *)calloc(BIGNUM_CAPACITY + 1, sizeof(uint64_t));
    ASSERT(big != NULL, "malloc");
    rc = bignum_init_from_array(&b, big, BIGNUM_CAPACITY + 1);
    ASSERT(rc == BIGNUM_ERROR_OVERFLOW, "array: overflow по len");
    OK("init_from_array — OVERFLOW при len > CAPACITY");
    free(big);

    /* Граница ровно CAPACITY — должно пройти */
    uint64_t full[BIGNUM_CAPACITY];
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) full[i] = (uint64_t)i + 1;
    rc = bignum_init_from_array(&b, full, BIGNUM_CAPACITY);
    ASSERT(rc == BIGNUM_SUCCESS, "array: len == CAPACITY");
    ASSERT(b.len == BIGNUM_CAPACITY, "array: len сохранён");
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == (uint64_t)i + 1, "array: граничное значение");
    }
    OK("init_from_array — граница CAPACITY");
}

/* ------------------------------------------------------------------ */
/*  bignum_is_zero                                                    */
/* ------------------------------------------------------------------ */

static void test_is_zero(void)
{
    bignum_t b;

    /* NULL — считается нулём */
    ASSERT(bignum_is_zero(NULL) == 1, "is_zero(NULL) == 1");
    OK("is_zero(NULL)");

    /* Только что инициализированный */
    bignum_init(&b);
    ASSERT(bignum_is_zero(&b) == 1, "is_zero после init");
    OK("is_zero после init");

    /* u64(0) — ноль */
    bignum_init_u64(&b, 0);
    ASSERT(bignum_is_zero(&b) == 1, "is_zero после u64(0)");
    OK("is_zero(u64(0))");

    /* u64(1) — не ноль */
    bignum_init_u64(&b, 1);
    ASSERT(bignum_is_zero(&b) == 0, "is_zero(u64(1)) == 0");
    OK("is_zero(u64(1))");

    /* UINT64_MAX — не ноль */
    bignum_init_u64(&b, UINT64_MAX);
    ASSERT(bignum_is_zero(&b) == 0, "is_zero(UINT64_MAX) == 0");
    OK("is_zero(UINT64_MAX)");

    /* "Ложный ноль" в "грязном" состоянии: len > 0, но words[len-1] == 0
     * — НЕ считается нулём по контракту (is_zero проверяет только len). */
    bignum_init(&b);
    b.words[0] = 1;
    b.len      = 1;
    bignum_clear_tail(&b, 1);
    /* Имитируем ручной сброс старшего слова без вызова normalize: */
    b.words[0] = 0;
    /* len всё ещё 1, words[0] == 0 — но is_zero вернёт 0 */
    ASSERT(bignum_is_zero(&b) == 0,
           "is_zero: ненормализованный 'ноль' != 0");
    /* После normalize станет нулём */
    bignum_normalize(&b);
    ASSERT(bignum_is_zero(&b) == 1, "is_zero после normalize → 1");
    OK("is_zero — поведение на ненормализованных данных");
}

/* ------------------------------------------------------------------ */
/*  bignum_clear_tail                                                 */
/* ------------------------------------------------------------------ */

static void test_clear_tail(void)
{
    bignum_t b;

    bignum_init(&b);
    /* Заполним мусором */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xDEADBEEFDEADBEEFULL;
    b.len = BIGNUM_CAPACITY;

    /* Очистка от индекса 0 — обнулит всё */
    bignum_clear_tail(&b, 0);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "clear_tail(0)");
    }
    OK("clear_tail(0)");

    /* Очистка от середины */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xCAFEBABECAFEBABEULL;
    bignum_clear_tail(&b, 16);
    for (size_t i = 0; i < 16; ++i) {
        ASSERT(b.words[i] == 0xCAFEBABECAFEBABEULL, "clear_tail: до start сохранено");
    }
    for (size_t i = 16; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "clear_tail: от start обнулено");
    }
    OK("clear_tail(mid)");

    /* start == CAPACITY — допустимый no-op */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xFF;
    bignum_clear_tail(&b, BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0xFF, "clear_tail(CAPACITY) — no-op");
    }
    OK("clear_tail(CAPACITY) — no-op");

    /* start > CAPACITY — допустимый no-op */
    bignum_clear_tail(&b, BIGNUM_CAPACITY + 100);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0xFF, "clear_tail(>CAPACITY) — no-op");
    }
    OK("clear_tail(>CAPACITY) — no-op");

    /* NULL — тихо */
    bignum_clear_tail(NULL, 0);
    OK("clear_tail(NULL) — без падения");
}

/* ------------------------------------------------------------------ */
/*  bignum_normalize                                                  */
/* ------------------------------------------------------------------ */

static void test_normalize(void)
{
    bignum_t b;

    /* NULL — тихо */
    bignum_normalize(NULL);
    OK("normalize(NULL)");

    /* len == 0 — должен обнулить хвост и не упасть на пустом массиве */
    bignum_init(&b);
    b.words[5] = 0x1234567890ABCDEFULL;   /* «мусор» за пределами len */
    bignum_normalize(&b);
    ASSERT(b.len == 0, "normalize(len=0): len сохраняется");
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "normalize(len=0): весь хвост обнулён");
    }
    OK("normalize(len=0) — хвост обнуляется");

    /* Срезка многих старших нулей подряд */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0;
    b.words[0] = 0x42;
    b.len      = BIGNUM_CAPACITY;
    bignum_normalize(&b);
    ASSERT(b.len == 1 && b.words[0] == 0x42,
           "normalize: срезка до words[0]");
    /* Хвост за пределами len должен быть обнулён */
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == 0, "normalize: хвост обнулён после срезки");
    }
    OK("normalize — срезка многих старших нулей");

    /* Инвариант: words[len-1] != 0 после normalize (если len > 0) */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xDEAD;
    b.words[10] = 0;
    b.len       = 11;
    bignum_normalize(&b);
    ASSERT(b.len == 10, "normalize: len ужался до 10");
    ASSERT(b.words[9] != 0, "normalize: инвариант words[len-1] != 0");
    OK("normalize — инвариант words[len-1] != 0");

    /* Число, занимающее ровно всю ёмкость — не должно пострадать */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = UINT64_MAX;
    b.len = BIGNUM_CAPACITY;
    bignum_normalize(&b);
    ASSERT(b.len == BIGNUM_CAPACITY, "normalize: full capacity сохранён");
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        ASSERT(b.words[i] == UINT64_MAX, "normalize: full capacity values");
    }
    OK("normalize — full capacity (2^2048-1)");

    /* Повторный вызов — идемпотентность */
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) b.words[i] = 0xABCDEF;
    b.words[5] = 0;
    b.len      = 6;
    bignum_normalize(&b);
    size_t first_len = b.len;
    bignum_normalize(&b);
    ASSERT(b.len == first_len, "normalize: идемпотентность по len");
    OK("normalize — идемпотентность");
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */

int main(void)
{
    test_init();
    test_init_u64();
    test_init_from_array();
    test_is_zero();
    test_clear_tail();
    test_normalize();

    if (fails == 0) printf("\nALL OK (%d проверок)\n", 0);
    else            printf("\n%d FAIL(s)\n", fails);
    return fails ? 1 : 0;
}