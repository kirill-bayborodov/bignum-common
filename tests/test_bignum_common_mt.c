/**
 * @file    test_bignum_common_mt.c
 * @brief   Многопоточный (MT) тест для базовых операций bignum_common.
 *
 * Что проверяется:
 * 1. Реентерабельность (Thread-safety): отсутствие скрытого глобального
 *    состояния. Каждый поток работает со своим экземпляром bignum_t.
 * 2. False Sharing & Memory Corruption: потоки агрессивно модифицируют
 *    свои bignum_t, лежащие в едином плотном массиве. Если ассемблерные
 *    вставки или C-код пишут за пределы структуры, это вызовет гонку
 *    данных (data race) с соседним потоком.
 * 3. Read-only contention: одновременный вызов bignum_is_zero для
 *    общих (shared) структур.
 *
 * Рекомендация для CI: запускать с -fsanitize=thread (TSAN).
 */

#include "bignum_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#define NUM_THREADS       8
#define NUM_ITERATIONS    500000

/* Атомарный флаг ошибки — единый для всех потоков. */
static atomic_int g_test_failed = ATOMIC_VAR_INIT(0);

/* Общие read-only данные для проверки конкурентного чтения */
static bignum_t g_shared_zero;
static bignum_t g_shared_nonzero;

/* Плотный массив для проверки false sharing. Каждый поток пишет только в свой индекс. */
static bignum_t g_thread_data[NUM_THREADS];

/* Макрос для удобного выхода при ошибке внутри потока */
#define THREAD_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("Thread %d FAIL: %s\n", thread_id, (msg)); \
            atomic_store(&g_test_failed, 1); \
            return NULL; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Worker: выполняет полный цикл операций над своим bignum_t         */
/* ------------------------------------------------------------------ */
static void* thread_func(void* arg)
{
    int thread_id = *(int*)arg;
    bignum_t* local_b = &g_thread_data[thread_id];

    /* Тестовые данные для init_from_array */
    uint64_t arr_data[3] = { 0x1111, 0x2222, 0x3333 };

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        /* Быстрый выход, если кто-то уже упал */
        if (atomic_load(&g_test_failed)) {
            return NULL;
        }

        /* 1. Конкурентное чтение общих данных */
        THREAD_ASSERT(bignum_is_zero(&g_shared_zero) == 1, "shared_zero is not zero");
        THREAD_ASSERT(bignum_is_zero(&g_shared_nonzero) == 0, "shared_nonzero is zero");

        /* 2. bignum_init */
        bignum_init(local_b);
        THREAD_ASSERT(local_b->len == 0, "init: len != 0");
        THREAD_ASSERT(bignum_is_zero(local_b) == 1, "init: is_zero != 1");

        /* 3. bignum_init_u64 */
        uint64_t magic = 0xDEADBEEF0000ULL + thread_id;
        bignum_init_u64(local_b, magic);
        THREAD_ASSERT(local_b->len == 1, "init_u64: len != 1");
        THREAD_ASSERT(local_b->words[0] == magic, "init_u64: wrong value");
        THREAD_ASSERT(bignum_is_zero(local_b) == 0, "init_u64: is_zero != 0");

        /* 4. bignum_init_from_array */
        arr_data[0] = thread_id; /* Уникализируем данные для потока */
        bignum_init_from_array(local_b, arr_data, 3);
        THREAD_ASSERT(local_b->len == 3, "init_from_array: len != 3");
        THREAD_ASSERT(local_b->words[2] == 0x3333, "init_from_array: wrong words[2]");

        /* 5. bignum_clear_tail */
        bignum_clear_tail(local_b, 1);
        THREAD_ASSERT(local_b->words[1] == 0, "clear_tail: words[1] != 0");
        THREAD_ASSERT(local_b->words[2] == 0, "clear_tail: words[2] != 0");
        /* len при этом не меняется по контракту clear_tail, он остается 3 */

        /* 6. bignum_normalize */
        /* Искусственно создаем "грязный" хвост и завышенный len */
        local_b->words[0] = 0x42 + thread_id;
        local_b->words[1] = 0;
        local_b->words[2] = 0;
        local_b->len = 10; 
        
        bignum_normalize(local_b);
        THREAD_ASSERT(local_b->len == 1, "normalize: len did not shrink to 1");
        THREAD_ASSERT(local_b->words[0] == (0x42ULL + thread_id), "normalize: wrong value");
        THREAD_ASSERT(local_b->words[9] == 0, "normalize: tail not cleared");
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
    printf("\n--- Running Multithreading Test for bignum_common ---\n");
    printf("Threads: %d, iterations per thread: %d, total ops: %d\n\n",
           NUM_THREADS, NUM_ITERATIONS, NUM_THREADS * NUM_ITERATIONS * 6);

    /* Инициализация общих read-only данных */
    bignum_init_u64(&g_shared_zero, 0);
    bignum_init_u64(&g_shared_nonzero, 0xCAFEBABE);

    /* Инициализация массива потоков */
    memset(g_thread_data, 0, sizeof(g_thread_data));

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("Launching threads...\n");
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return -1;
        }
    }

    /* Проверка результатов */
    if (atomic_load(&g_test_failed)) {
        printf("--- MT test FAILED: race, corruption or wrong result detected ---\n");
        return -1;
    }

    printf("\nCoverage summary:\n");
    printf("  [x] Concurrent bignum_is_zero on shared data\n");
    printf("  [x] Thread-local bignum_init & bignum_init_u64\n");
    printf("  [x] Thread-local bignum_init_from_array\n");
    printf("  [x] Thread-local bignum_clear_tail\n");
    printf("  [x] Thread-local bignum_normalize\n");
    printf("  [x] False sharing / memory bounds check via dense array\n");

    printf("\n--- MT test PASSED ---\n");
    return 0;
}
