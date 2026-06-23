/**
 * @file    bignum_common.c
 * @author  git@bayborodov.com
 * @version 1.0.7
 * @date    17.06.2026
 *
 * @brief   Реализация ядра bignum: инициализация, нормализация, is_zero.
 *
 * @details
 *   Парсинг строк (hex/dec) намеренно вынесен в модуль `bignum_helpers`
 *  
 * @history
 *   - rev. 0 (19.06.2026): Первоначальная реализация.
 *   - rev. 1 (23.06.2026): В bignum_normalize добавлена защита от мусорного len 
 */

#include "bignum_common.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/*  Инициализация                                                     */
/* ------------------------------------------------------------------ */

void bignum_init(bignum_t *x)
{
    if (x == NULL) {
        return;
    }
    memset(x, 0, sizeof(*x));
}

void bignum_init_u64(bignum_t *b, uint64_t val)
{
    if (b == NULL) {
        return;
    }
    bignum_init(b);
    if (val != 0U) {
        b->words[0] = val;
        b->len      = 1U;
    }
}

int bignum_init_from_array(bignum_t *dst,
                           const uint64_t *src,
                           size_t src_len)
{
    if (dst == NULL || src == NULL) {
        return BIGNUM_ERROR_NULL_ARG;
    }
    if (src_len > BIGNUM_CAPACITY) {
        return BIGNUM_ERROR_OVERFLOW;
    }

    memset(dst->words, 0, sizeof(dst->words));
    if (src_len > 0U) {
        memcpy(dst->words, src, src_len * BIGNUM_WORD_SIZE);
    }
    dst->len = src_len;

    while (dst->len > 0U && dst->words[dst->len - 1U] == 0ULL) {
        --dst->len;
    }
    return BIGNUM_SUCCESS;
}

int bignum_is_zero(const bignum_t *x)
{
    return (x == NULL) || (x->len == 0U);
}

/* ------------------------------------------------------------------ */
/*  Нормализация и обслуживание                                       */
/* ------------------------------------------------------------------ */

void bignum_clear_tail(bignum_t *x, size_t start)
{
    if (x == NULL) {
        return;
    }
    if (start >= BIGNUM_CAPACITY) {
        return;
    }
    memset(x->words + start, 0,
           (BIGNUM_CAPACITY - start) * BIGNUM_WORD_SIZE);
}

void bignum_normalize(bignum_t *x)
{
    if (x == NULL) {
        return;
    }

    /* Защита от мусорного значения len, превышающего вместимость */
    if (x->len > BIGNUM_CAPACITY) {
        x->len = BIGNUM_CAPACITY;
    }

    if (x->len == 0U) {
        bignum_clear_tail(x, 0U);
        return;
    }

    while (x->len > 0U && x->words[x->len - 1U] == 0ULL) {
        --x->len;
    }
    bignum_clear_tail(x, x->len);
}
