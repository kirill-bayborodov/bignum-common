/**
 * @file    bignum.h
 * @author  git@bayborodov.com
 * @version 1.0.2
 * @date    03.10.2025
 *
 * @brief   Определение базовой структуры для арифметики с большими числами.
 *
 * @details
 *   Эта структура `bignum_t` является основой для всех высокоточных
 *   целочисленных вычислений в алгоритме форматирования `double`.
 *   Размер массива `words` (32 * 64 = 2048 бита) выбран для покрытия
 *   всех крайних случаев, возникающих при преобразовании `double`.
 *
 * @history
 *   - rev. 1 (01.08.2025): Первоначальное создание.
 *   - rev. 2 (03.10.2025): Дополнение для github
 */
#ifndef BIGNUM_H
#define BIGNUM_H

#include <stdint.h>
#include <stddef.h>

#define BIGNUM_CAPACITY 32 // 32 * (8*8) =  2048 бит
#define BIGNUM_WORD_SIZE 8 // sizeof(uint64_t) байт

/**
 * @brief Структура для представления большого беззнакового целого числа.
 */
typedef struct {
    /** Массив 64-битных "слов" для хранения числа. */
    uint64_t words[BIGNUM_CAPACITY];
    /** Количество используемых слов. words[len-1] не может быть 0, кроме числа 0. */
    size_t len;
} bignum_t;

typedef enum {
    BIGNUM_SUCCESS = 0,
    BIGNUM_ERROR_NULL_ARG = -1,
    BIGNUM_ERROR_OVERFLOW = -2
} bignum_status_t;


#endif // BIGNUM_H
