/**
 * @file    bignum_common.h
 * @author  git@bayborodov.com
 * @version 1.0.8
 * @date    17.06.2026
 *
 * @brief   Основные функции ядра для арифметики с большими числами.
 *
 * @details
 *   Структура `bignum_t` является основой для всех высокоточных
 *   целочисленных вычислений в алгоритме форматирования `double`.
 *   Размер массива `words` (32 * 64 = 2048 бита) выбран для покрытия
 *   всех крайних случаев, возникающих при преобразовании `double`.
 *
 *
 * @history
 *   - rev. 1 (01.08.2025): Первоначальное создание.
 *   - rev. 2 (03.10.2025): Дополнение для github.
 *   - rev. 3 (17.06.2026): Добавлены функции инициализации, нормализации.
 *   - rev. 4 (17.06.2026): Реализация вынесена в bignum_common.c.
 *   - rev. 5 (17.06.2026): Реализован парсинг hex/dec;
 *                          bignum_is_zero перенесён в .c.
 *   - rev. 6 (17.06.2026): Парсинг вынесен в `bignum_helpers`.
 *   - rev. 7 (17.06.2026): BIGNUM_ERROR_BAD_BASE/PARSE/EMPTY вынесены
 *                          в `bignum_helpers.h`; bignum.c → bignum_common.c.
 *   - rev. 8 (18.06.2026): Сигнатуры для основныx функций ядра перенесены в bignum_common.h
 */
#ifndef BIGNUM_COMMON_H
#define BIGNUM_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "bignum.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------ */
/*  Основные функции ядра — реализация в bignum_common.[c.|.asm]      */
/* ------------------------------------------------------------------ */

/** Обнуляет bignum_t (len = 0, words = 0). */
void bignum_init(bignum_t *x);

/** Инициализирует bignum_t из 64-битного числа. */
void bignum_init_u64(bignum_t *b, uint64_t val);

/**
 * @brief Инициализирует bignum_t из массива uint64_t (младшее слово — first).
 * @return BIGNUM_SUCCESS, BIGNUM_ERROR_NULL_ARG, BIGNUM_ERROR_OVERFLOW.
 */
int bignum_init_from_array(bignum_t *dst,
                           const uint64_t *src,
                           size_t src_len);

/** Возвращает 1, если x == NULL или x->len == 0, иначе 0. */
int bignum_is_zero(const bignum_t *x);

/**
 * @brief Обнуляет «хвост» массива words от индекса start (включительно)
 *        до конца статического буфера BIGNUM_CAPACITY.
 */
void bignum_clear_tail(bignum_t *x, size_t start);

/**
 * @brief Удаляет старшие нулевые слова и обнуляет хвост.
 *        Должна вызываться после любой арифметической операции,
 *        чтобы гарантировать отсутствие неинициализированных данных.
 */
void bignum_normalize(bignum_t *x);

/*
 * Замечание: парсинг строк (hex/dec) живёт в модуле `bignum_helpers` —
 * подключайте его явно через
 *   #include "bignum_helpers.h"
 * если нужна функция `bignum_init_from_string`.
 */

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_COMMON_H */