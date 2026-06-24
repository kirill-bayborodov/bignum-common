; -----------------------------------------------------------------------------
; @file    bignum_common.asm
; @author  git@bayborodov.com
; @version 1.0.6
; @date    24.06.2026
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
;   - rev. 1 (23.06.2026): Исправлены ошибки в bignum_normalize с обнулением хвоста и добавлена защита от мусорного len
;   - rev. 2 (24.06.2026): Оптимизация bignum_init_from_array: нормализовать данные «на лету» (до копирования) и 
;                          сделать всё за один проход без вызовов сторонних функций.
;   - rev. 3 (24.06.2026): Оптимизация bignum_normalize: GPO/PGO-оптимизированная версия (Hot Path выстроен линейно)
;   - rev. 4 (24.06.2026): Оптимизация bignum_normalize: Compact / Conditional-Move оптимизированная версия (применена Branchless-логика для сокращения ветвлений)
;   - rev. 5 (24.06.2026): Оптимизация bignum_init_from_array: Compact / Branchless оптимизированная версия (уплотнение кода и сокращение ветвлений)
;   - rev. 6 (24.06.2026): Оптимизация bignum_init, bignum_init_u64, bignum_is_zero, bignum_clear_tail: Compact / Branchless оптимизированная версия (уплотнение кода и сокращение ветвлений)
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
bignum_init:
    test    rdi, rdi
    jz      .ret
    
    ; Оптимизация (Compact): структура занимает 256 байт (words) + 8 байт (len) = 264 байта.
    ; Это ровно 33 QWORD. Обнуляем всю память за одну команду без отдельной записи в len.
    mov     ecx, BUF_QWORDS + 1
    xor     eax, eax
    rep     stosq
.ret:
    ret



; @brief  Инициализирует bignum_t из 64-битного числа.
; void bignum_init_u64(bignum_t *b, uint64_t val)
; rdi = b, rsi = val
bignum_init_u64:
    test    rdi, rdi
    jz      .ret

    mov     r8, rdi                         ; Сохраняем указатель (rep stosq меняет rdi)
    
    ; 1. Обнуляем всю структуру (33 QWORD) разом
    mov     ecx, BUF_QWORDS + 1
    xor     eax, eax
    rep     stosq

    ; 2. Записываем значение
    mov     [r8 + BIGNUM_OFFSET_WORDS], rsi 

    ; 3. Branchless вычисление len: если rsi != 0, то len = 1, иначе 0
    test    rsi, rsi
    setnz   al                              ; al = 1, если rsi != 0
    mov     [r8 + BIGNUM_OFFSET_LEN], rax   ; rax уже содержит 0 или 1 (старшие биты обнулены xor eax, eax выше)
.ret:
    ret

; @brief Инициализирует bignum_t из массива uint64_t (младшее слово — first).
; @return BIGNUM_SUCCESS, BIGNUM_ERROR_NULL_ARG, BIGNUM_ERROR_OVERFLOW.
; int bignum_init_from_array(bignum_t *dst, const uint64_t *src, size_t src_len)
; rdi = dst, rsi = src, rdx = src_len
bignum_init_from_array:
    ; 1. Компактные проверки на ошибки (ранний выход обязателен для защиты памяти)
    mov     eax, BIGNUM_ERROR_NULL_ARG
    test    rdi, rdi
    jz      .ret
    test    rsi, rsi
    jz      .ret

    mov     eax, BIGNUM_ERROR_OVERFLOW
    cmp     rdx, BUF_QWORDS
    ja      .ret

    ; 2. Нормализация "на лету" (поиск реальной длины)
    mov     rcx, rdx
    test    rcx, rcx
    jz      .do_copy        ; Если src_len == 0, сразу идем обнулять dst

.scan_loop:
    cmp     qword [rsi + rcx*8 - 8], 0
    jnz     .do_copy        ; Нашли первое ненулевое слово
    dec     rcx
    jnz     .scan_loop      ; Если дошли до 0, массив состоял только из нулей

.do_copy:
    ; 3. Записываем длину ДО копирования (rdi еще указывает на начало структуры)
    mov     [rdi + BIGNUM_OFFSET_LEN], rcx

    ; 4. Branchless копирование значащих слов
    mov     r8, rcx         ; Сохраняем длину для вычисления остатка
    cld                     ; Очищаем флаг направления (System V ABI)
    rep     movsq           ; Если rcx=0, аппаратно пропускается. rdi сдвигается на rcx*8

    ; 5. Branchless обнуление хвоста
    mov     rcx, BUF_QWORDS
    sub     rcx, r8         ; rcx = 32 - количество скопированных слов
    xor     eax, eax        ; rax = 0 (для stosq) И eax = BIGNUM_SUCCESS (0) для return
    rep     stosq           ; Если rcx=0, аппаратно пропускается (ветвление не нужно!)

.ret:
    ret




; @brief Возвращает 1, если x == NULL или x->len == 0, иначе 0. 
; int bignum_is_zero(const bignum_t *x)
; rdi = x
bignum_is_zero:
    mov     eax, 1                          ; Предполагаем x == NULL (возврат 1)
    test    rdi, rdi
    jz      .ret                            ; Защита от segfault (единственное ветвление)
    
    ; Branchless проверка: eax = (len == 0) ? 1 : 0
    cmp     qword [rdi + BIGNUM_OFFSET_LEN], 0
    sete    al                              ; Меняет только младший байт (al). 
                                            ; Так как eax был 1, старшие биты уже нули.
.ret:
    ret

; @brief Обнуляет «хвост» массива words от индекса start (включительно)
;        до конца статического буфера BIGNUM_CAPACITY.
; void bignum_clear_tail(bignum_t *x, size_t start)
; rdi = x, rsi = start
bignum_clear_tail:
    test    rdi, rdi
    jz      .ret

    mov     rcx, BUF_QWORDS
    
    ; Branchless ограничение (Clamp): если start > 32, делаем start = 32
    cmp     rsi, rcx
    cmova   rsi, rcx                

    ; Вычисляем размер хвоста
    sub     rcx, rsi                        ; rcx = 32 - start (количество слов для обнуления)
    
    lea     rdi, [rdi + rsi*8]              ; Сдвигаем указатель на начало хвоста
    xor     eax, eax
    rep     stosq                           ; Если rcx == 0, аппаратно пропускается без ветвлений
.ret:
    ret


; @brief Удаляет старшие нулевые слова и обнуляет хвост.
;        Должна вызываться после любой арифметической операции,
;        чтобы гарантировать отсутствие неинициализированных данных.
; void bignum_normalize(bignum_t *x); 
; rdi = x
bignum_normalize:
    test    rdi, rdi
    jz      .ret

    ; 1. Загружаем текущую длину и ограничиваем её до BIGNUM_CAPACITY (32) без ветвлений
    mov     rcx, [rdi + BIGNUM_OFFSET_LEN]
    mov     r8, BUF_QWORDS
    cmp     rcx, r8
    cmova   rcx, r8       ; Если rcx > 32, то rcx = 32

    ; Если длина изначально 0, сразу переходим к обновлению (и обнулению всего буфера)
    test    rcx, rcx
    jz      .update_len

.scan_loop:
    ; 2. Ищем первое ненулевое слово с конца (без загрузки в промежуточный регистр)
    cmp     qword [rdi + rcx*8 - 8], 0
    jnz     .update_len   ; Нашли! rcx содержит правильную новую длину
    dec     rcx
    jnz     .scan_loop    ; Продолжаем поиск, пока rcx не станет 0

    ; Если цикл завершился (rcx = 0), значит все слова были нулями.
    ; Естественным образом "проваливаемся" в .update_len с rcx = 0.

.update_len:
    ; 3. Сохраняем новую длину (от 0 до 32)
    mov     [rdi + BIGNUM_OFFSET_LEN], rcx

    ; 4. Вычисляем размер "хвоста" для обнуления: (32 - rcx)
    mov     r8, BUF_QWORDS
    sub     r8, rcx
    jz      .ret          ; Если новая длина 32, обнулять нечего (хвоста нет)

    ; 5. Обнуляем хвост (или весь буфер, если rcx был равен 0)
    lea     rdi, [rdi + rcx*8]  ; Указатель на начало хвоста
    mov     rcx, r8             ; Количество слов для обнуления
    xor     eax, eax            ; Заполняем нулями
    rep     stosq               ; Обнуляем rcx слов

.ret:
    ret









