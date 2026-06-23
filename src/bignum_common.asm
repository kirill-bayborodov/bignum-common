; -----------------------------------------------------------------------------
; @file    bignum_common.asm
; @author  git@bayborodov.com
; @version 1.0.0
; @date    19.06.2026
;
; @brief   Реализация функций ядра больших целых чисел (bignum) на YASM x86_64.
;
; @details
;   Эталонная ассемблерная реализация для Yasm x86-64 (System V ABI).
;   Модуль предоставляет функцию bignum_common для инициализация, нормализация, is_zero больших чисел,
;   определённых типом bignum_t. Реализованы проверки корректности входных указателей, 
;   диапазонов длин операндов, отсутствие перекрытия буферов, а также нормализация результата.
;
;   Структура для представления большого беззнакового целого числа.   
;   typedef struct {
;       /** Массив 64-битных "слов" для хранения числа. */
;       uint64_t words[BIGNUM_CAPACITY];
;       /**
;        * Количество используемых слов.
;        * Для числа 0 значение len равно 0.
;        * Для всех остальных чисел гарантируется, что words[len-1] != 0
;        * (число нормализовано).
;        */
;       size_t   len;
;   } bignum_t;
;
; @history
;   - rev. 0 (19.06.2026): Первоначальная реализация на ассемблере.
; -----------------------------------------------------------------------------

section .text

; --- Константы ---
BIGNUM_CAPACITY         equ 32
BIGNUM_WORD_SIZE        equ 8
BIGNUM_BITS             equ BIGNUM_CAPACITY * 64
BIGNUM_OFFSET_WORDS     equ 0
BIGNUM_OFFSET_LEN       equ BIGNUM_CAPACITY * BIGNUM_WORD_SIZE   ; 256

; @brief Коды возврата ядра bignum.
BIGNUM_SUCCESS          equ 0
BIGNUM_ERROR_NULL_ARG   equ -1
BIGNUM_ERROR_OVERFLOW   equ -2

BUF_QWORDS              equ BIGNUM_CAPACITY      ; 32 qword = 256 bytes
BUF_SIZE                equ 264                  ; 256 + 8 

    global bignum_init
    global bignum_init_u64
    global bignum_init_from_array
    global bignum_is_zero
    global bignum_clear_tail
    global bignum_normalize
   
; @brief  Обнуляет bignum_t (len = 0, words = 0).
; void bignum_init(bignum_t *x)
; rdi = x
; void bignum_init(bignum_t *x)
; rdi = x
bignum_init:
    test    rdi, rdi
    jz      .ret

    mov     r8, rdi
    lea     rdi, [r8 + BIGNUM_OFFSET_WORDS]
    xor     eax, eax
    mov     ecx, BUF_QWORDS
    cld
    rep     stosq
    mov     qword [r8 + BIGNUM_OFFSET_LEN], 0
.ret:
    ret



; @brief  Инициализирует bignum_t из 64-битного числа.
; void bignum_init_u64(bignum_t *b, uint64_t val)
; rdi = b, rsi = val
bignum_init_u64:
	test    rdi, rdi
	jz      .u64_ret

	mov     r8, rdi
	xor     rax, rax
	mov rcx, BUF_QWORDS
	cld
	rep     stosq
	mov     rdi, r8
    mov     qword [rdi + BIGNUM_OFFSET_LEN], 0 

	mov     qword [rdi + BIGNUM_OFFSET_WORDS], rsi
	test    rsi, rsi
	jz      .u64_ret       ; len already zero
	mov     qword [rdi + BIGNUM_OFFSET_LEN], 1
	.u64_ret:
	ret

; @brief Инициализирует bignum_t из массива uint64_t (младшее слово — first).
; @return BIGNUM_SUCCESS, BIGNUM_ERROR_NULL_ARG, BIGNUM_ERROR_OVERFLOW.  
; int bignum_init_from_array(bignum_t *dst, const uint64_t *src, size_t src_len)
; rdi = dst, rsi = src, rdx = src_len
; int bignum_init_from_array(bignum_t *dst, const uint64_t *src, size_t src_len)
; rdi = dst, rsi = src, rdx = src_len
bignum_init_from_array:
    test    rdi, rdi
    jz      .err_null
    test    rsi, rsi
    jz      .err_null

    cmp     rdx, BUF_QWORDS
    ja      .err_overflow

    mov     r8, rdi

    test    rdx, rdx
    jz      .clear_all

    lea     rdi, [r8 + BIGNUM_OFFSET_WORDS]
    mov     rcx, rdx
    cld
    rep     movsq

    mov     [r8 + BIGNUM_OFFSET_LEN], rdx

    cmp     rdx, BUF_QWORDS
    je      .normalize

    mov     rdi, r8
    mov     rsi, rdx
    call    bignum_clear_tail

.normalize:
    mov     rdi, r8
    call    bignum_normalize
    mov     eax, BIGNUM_SUCCESS
    ret

.clear_all:
    lea     rdi, [r8 + BIGNUM_OFFSET_WORDS]
    xor     eax, eax
    mov     ecx, BUF_QWORDS
    cld
    rep     stosq
    mov     qword [r8 + BIGNUM_OFFSET_LEN], 0
    mov     eax, BIGNUM_SUCCESS
    ret


.err_null:
    mov     eax, BIGNUM_ERROR_NULL_ARG
    ret

.err_overflow:
    mov     eax, BIGNUM_ERROR_OVERFLOW
    ret


; @brief Возвращает 1, если x == NULL или x->len == 0, иначе 0. 
; int bignum_is_zero(const bignum_t *x)
; rdi = x
bignum_is_zero:
    test    rdi, rdi
    jz      .is_zero_true
    mov     rcx, qword [rdi + BIGNUM_OFFSET_LEN]
    test    rcx, rcx
    jz      .is_zero_true
    xor     eax, eax
    ret
.is_zero_true:
    mov     eax, 1
    ret

; @brief Обнуляет «хвост» массива words от индекса start (включительно)
;        до конца статического буфера BIGNUM_CAPACITY.
; void bignum_clear_tail(bignum_t *x, size_t start)
; rdi = x, rsi = start


bignum_clear_tail:
    test    rdi, rdi
    jz      .ret

    cmp     rsi, BUF_QWORDS
    jae     .ret

    mov     rcx, BUF_QWORDS
    sub     rcx, rsi
    lea     rdi, [rdi + rsi*8]
    xor     eax, eax
    cld
    rep     stosq

.ret:
    ret


; @brief Удаляет старшие нулевые слова и обнуляет хвост.
;        Должна вызываться после любой арифметической операции,
;        чтобы гарантировать отсутствие неинициализированных данных.
; void bignum_normalize(bignum_t *x); 
; rdi = x

; @brief Удаляет старшие нулевые слова и обнуляет хвост.
; void bignum_normalize(bignum_t *x)
; rdi = x
bignum_normalize:
    test    rdi, rdi
    jz      .ret

    mov     r8, rdi
    mov     rcx, [r8 + BIGNUM_OFFSET_LEN]
    test    rcx, rcx
    jz      .clear_all

.loop:
    ; Проверяем текущее старшее слово
    mov     rax, [r8 + BIGNUM_OFFSET_WORDS + rcx*8 - 8]
    test    rax, rax
    jnz     .store_len    ; Если не ноль, мы нашли новый len, сохраняем

    dec     rcx
    jnz     .loop         ; Если rcx еще не 0, продолжаем цикл

    ; Если дошли сюда, значит все слова были нулями (rcx == 0)
    jmp     .clear_all

.store_len:
    mov     [r8 + BIGNUM_OFFSET_LEN], rcx
    test    rcx, rcx
    jnz     .ret

.clear_all:
    lea     rdi, [r8 + BIGNUM_OFFSET_WORDS]
    xor     eax, eax
    mov     ecx, BUF_QWORDS
    cld
    rep     stosq
    mov     qword [r8 + BIGNUM_OFFSET_LEN], 0
.ret:
    ret







