/**
 * @file    bench_bignum_common.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    17.06.2026
 *
 * @brief   Бенчмарк ядра bignum (single-threaded).
 *
 * Использование:
 *   ./bench_bignum_common <REPORT_NAME>
 *
 * Формат отчёта совместим со стилем perf-report:
 *   benchmarks/reports/<REPORT_NAME>_st.txt
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif

#include "bignum_common.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WARMUP_ITERS  10000U
#define MEASURE_ITERS 100000U

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

typedef enum {
    VAR_CLEAN  = 0,
    VAR_SHORT,
    VAR_MID,
    VAR_FULL,
    VAR_RANDOM,
    VAR__COUNT
} variant_t;

static const char *variant_name(variant_t v)
{
    switch (v) {
        case VAR_CLEAN:  return "clean (all zeros)";
        case VAR_SHORT:  return "short (1 word)";
        case VAR_MID:    return "mid (16 words)";
        case VAR_FULL:   return "full (32 words, UINT64_MAX)";
        case VAR_RANDOM: return "random";
        default:         return "?";
    }
}

typedef struct {
    const char  *label;
    variant_t    variant;
    uint64_t     total_iters;
    uint64_t     warmup_iters;
    uint64_t     measure_iters;
    double       ns_per_op;
    double       ops_per_sec;
    double       bytes_per_sec;
} bench_result_t;

static void dirty_buffer(bignum_t *b, variant_t v, uint64_t iter)
{
    uint64_t s = (iter + 1) * 0xBF58476D1CE4E5B9ULL;
    s ^= s >> 30; s *= 0xBF58476D1CE4E5B9ULL;
    s ^= s >> 27; s *= 0x94D049BB133111EBULL;
    s ^= s >> 31;

    b->len = BIGNUM_CAPACITY;

    switch (v) {
        case VAR_CLEAN:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) {
                b->words[k] = 0ULL;
            }
            break;

        case VAR_SHORT:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) {
                b->words[k] = (k == 0) ? 0xDEADBEEFULL : 0ULL;
            }
            break;

        case VAR_MID:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) {
                b->words[k] = (k < BIGNUM_CAPACITY / 2)
                    ? ((uint64_t)k + 1)
                    : 0ULL;
            }
            break;

        case VAR_FULL:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) {
                b->words[k] = UINT64_MAX;
            }
            break;

        case VAR_RANDOM:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) {
                s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
                b->words[k] = s * 0x2545F4914F6CDD1DULL;
            }
            break;

        case VAR__COUNT:
        default:
            for (size_t k = 0; k < BIGNUM_CAPACITY; ++k) b->words[k] = 0ULL;
            break;
    }
}

static bench_result_t bench_normalize(variant_t v)
{
    bench_result_t r = {0};
    r.label          = variant_name(v);
    r.variant        = v;
    r.warmup_iters   = WARMUP_ITERS;
    r.measure_iters  = MEASURE_ITERS;
    r.total_iters    = WARMUP_ITERS + MEASURE_ITERS;

    bignum_t b;
    dirty_buffer(&b, v, 0);

    for (uint64_t i = 0; i < WARMUP_ITERS; ++i) {
        dirty_buffer(&b, v, i);
        bignum_normalize(&b);
    }

    uint64_t t0 = now_ns();
    for (uint64_t i = 0; i < MEASURE_ITERS; ++i) {
        dirty_buffer(&b, v, i);
        bignum_normalize(&b);
    }
    uint64_t t1 = now_ns();

    uint64_t dt = t1 - t0;
    r.ns_per_op    = (double)dt / (double)MEASURE_ITERS;
    r.ops_per_sec  = (double)MEASURE_ITERS * 1e9 / (double)dt;

    size_t bytes = 2U * BIGNUM_CAPACITY * sizeof(uint64_t);
    r.bytes_per_sec = (double)bytes * (double)MEASURE_ITERS * 1e9 / (double)dt;

    return r;
}

static void print_header(FILE *fp, const char *mode, const char *report_name)
{
    fprintf(fp,
        "bignum_normalize — benchmarks/reports/%s_%s.txt (CONFIG=%s)\n",
        report_name, mode,
#ifdef NDEBUG
        "release"
#else
        "debug"
#endif
    );
    fprintf(fp, "==================================================================\n");
    fprintf(fp, "variant               ns/op      ops/sec    bytes/sec     samples\n");
    fprintf(fp, "------------------------------------------------------------------\n");
}

static void print_row(FILE *fp, const bench_result_t *r)
{
    fprintf(fp, "%-20s %8.2f %12.2f Mops %9.2f MB/s   %" PRIu64 "\n",
            r->label,
            r->ns_per_op,
            r->ops_per_sec / 1e6,
            r->bytes_per_sec / (1024.0 * 1024.0),
            r->measure_iters);
}

static void print_footer(FILE *fp)
{
    fprintf(fp, "\n");
    fprintf(fp, "Notes:\n");
    fprintf(fp, "  - ops/sec — нормировано на один поток (MT: сумма по потокам).\n");
    fprintf(fp, "  - bytes/sec — оценочная пропускная способность по массиву\n");
    fprintf(fp, "    words (BIGNUM_CAPACITY * 8 = %d байт) на одну операцию.\n",
            BIGNUM_CAPACITY * 8);
    fprintf(fp, "  - samples — количество замеряемых итераций на поток.\n");
}

int main(int argc, char **argv)
{
    const char *report_name = (argc > 1) ? argv[1] : "current";

    char path[512];
    snprintf(path, sizeof(path),
             "benchmarks/reports/%s_st.txt", report_name);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    print_header(fp, "st", report_name);

    for (int v = 0; v < (int)VAR__COUNT; ++v) {
        bench_result_t r = bench_normalize((variant_t)v);
        print_row(fp, &r);
        printf("%-20s %8.2f ns/op\n", r.label, r.ns_per_op);
    }
    print_footer(fp);
    fclose(fp);

    printf("\nReport: %s\n", path);
    return 0;
}
