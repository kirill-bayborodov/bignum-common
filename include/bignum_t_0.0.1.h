/**
 * @file    bignum_t_0.0.1.h
 * @author  git@bayborodov.com
 * @version 0.0.1
 * @date    01.10.2025
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
 *   - rev. 2 (01.10.2025): Дополнение для github
 */
#ifndef BIGNUM_T_H
#define BIGNUM_T_H

#include <stdint.h>
#include <stddef.h>

#define BIGNUM_CAPACITY 32 // 32 * 64 =  2048 бит
#define BIGNUM_WORD_SIZE 64

/**
 * @brief Структура для представления большого беззнакового целого числа.
 */
typedef struct {
    /** Массив 64-битных "слов" для хранения числа. */
    uint64_t words[BIGNUM_CAPACITY];
    /** Количество используемых слов. words[len-1] не может быть 0, кроме числа 0. */
    int len;
} bignum_t;

typedef enum {
    BIGNUM_SUCCESS = 0,
    BIGNUM_ERROR_NULL_ARG = -1,
    BIGNUM_ERROR_OVERFLOW = -2
} bignum_status_t;


#endif // BIGNUM_T_H
