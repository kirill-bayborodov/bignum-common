/**
 * @file    bignum.h
 * @author  git@bayborodov.com
 * @version 1.0.8
 * @date    17.06.2026
 *
 * @brief   Определение базовой структуры для арифметики с большими числами.
 *
 * @details
 *   Структура `bignum_t` является основой для всех высокоточных
 *   целочисленных вычислений в алгоритме форматирования `double`.
 *   Размер массива `words` (32 * 64 = 2048 бита) выбран для покрытия
 *   всех крайних случаев, возникающих при преобразовании `double`.
 *
 *   Коды ошибок, относящиеся только к парсингу строк
 *   (BIGNUM_ERROR_BAD_BASE/PARSE/EMPTY), объявлены в `bignum_helpers.h`.
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
#ifndef BIGNUM_H
#define BIGNUM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIGNUM_CAPACITY 32   /* 32 * 64 = 2048 бит */
#define BIGNUM_WORD_SIZE 8   /* sizeof(uint64_t) байт  */

/**
 * @brief Коды возврата ядра bignum.
 *
 * Коды, специфичные для парсинга строк (BIGNUM_ERROR_BAD_BASE,
 * BIGNUM_ERROR_PARSE, BIGNUM_ERROR_EMPTY), объявлены в `bignum_helpers.h`.
 */
typedef enum {
    BIGNUM_SUCCESS         =  0,
    BIGNUM_ERROR_NULL_ARG  = -1,
    BIGNUM_ERROR_OVERFLOW  = -2
} bignum_status_t;

/**
 * @brief Структура для представления большого беззнакового целого числа.
 */
typedef struct {
    /** Массив 64-битных "слов" для хранения числа. */
    uint64_t words[BIGNUM_CAPACITY];
    /**
     * Количество используемых слов.
     * Для числа 0 значение len равно 0.
     * Для всех остальных чисел гарантируется, что words[len-1] != 0
     * (число нормализовано).
     */
    size_t   len;
} bignum_t;


#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_H */