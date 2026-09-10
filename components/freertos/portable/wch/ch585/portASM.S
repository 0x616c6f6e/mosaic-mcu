/*
 * Based on the WCH CH585 FreeRTOS V10.5.1 QingKe port.
 * Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define portCONTEXT_SIZE (32 * 4)

.global xPortStartFirstTask
.global pxPortInitialiseStack
.global SW_Handler
.extern pxCurrentTCB
.extern vTaskSwitchContext

.global handle_reset
.extern _vector_base
.extern highcode_init
.extern main

.section .handle_reset,"ax",@progbits
.align 1
.func
handle_reset:
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, _eusrstack

    la a0, _highcode_init_lma
    la a1, _highcode_init_vma_start
    la a2, _highcode_init_vma_end
1:
    bgeu a1, a2, 2f
    lw t0, 0(a0)
    sw t0, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    j 1b
2:
    la a0, freertos_highcode_init
    jr a0
.endfunc

.section .highcode_init.freertos,"ax",@progbits
.align 2
.func
freertos_highcode_init:
    call highcode_init

    la a0, _highcode_lma
    la a1, _highcode_vma_start
    la a2, _highcode_vma_end
1:
    bgeu a1, a2, 2f
    lw t0, 0(a0)
    sw t0, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    j 1b
2:
    la a0, _data_lma
    la a1, _data_vma
    la a2, _edata
1:
    bgeu a1, a2, 2f
    lw t0, 0(a0)
    sw t0, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    j 1b
2:
    la a0, _sbss
    la a1, _ebss
1:
    bgeu a0, a1, 2f
    sw zero, 0(a0)
    addi a0, a0, 4
    j 1b
2:
    li t0, 0x25
    csrw 0xbc0, t0
    li t0, 0x03
    csrw 0x804, t0
    csrw 0xbc1, zero

    li t0, 0x1880
    csrw mstatus, t0
    la t0, _vector_base
    ori t0, t0, 0x03
    csrw mtvec, t0
    la t0, main
    csrw mepc, t0
    mret
.endfunc

.section .highcode.SW_Handler,"ax",@progbits
.align 2
.func
SW_Handler:
    addi sp, sp, -portCONTEXT_SIZE
    sw x1,  1 * 4(sp)
    sw x5,  2 * 4(sp)
    sw x6,  3 * 4(sp)
    sw x7,  4 * 4(sp)
    sw x8,  5 * 4(sp)
    sw x9,  6 * 4(sp)
    sw x10, 7 * 4(sp)
    sw x11, 8 * 4(sp)
    sw x12, 9 * 4(sp)
    sw x13, 10 * 4(sp)
    sw x14, 11 * 4(sp)
    sw x15, 12 * 4(sp)
    sw x16, 13 * 4(sp)
    sw x17, 14 * 4(sp)
    sw x18, 15 * 4(sp)
    sw x19, 16 * 4(sp)
    sw x20, 17 * 4(sp)
    sw x21, 18 * 4(sp)
    sw x22, 19 * 4(sp)
    sw x23, 20 * 4(sp)
    sw x24, 21 * 4(sp)
    sw x25, 22 * 4(sp)
    sw x26, 23 * 4(sp)
    sw x27, 24 * 4(sp)
    sw x28, 25 * 4(sp)
    sw x29, 26 * 4(sp)
    sw x30, 27 * 4(sp)
    sw x31, 28 * 4(sp)
    sw x4,  29 * 4(sp)

    li a1, 0x20
    csrs 0x804, a1

    la t0, pxCurrentTCB
    lw t0, 0(t0)
    sw sp, 0(t0)

    csrr a1, mepc
    sw a1, 0(sp)

    call vTaskSwitchContext

    la t1, pxCurrentTCB
    lw t1, 0(t1)
    lw sp, 0(t1)

    lw t0, 0(sp)
    csrw mepc, t0

    lw x1,  1 * 4(sp)
    lw x5,  2 * 4(sp)
    lw x6,  3 * 4(sp)
    lw x7,  4 * 4(sp)
    lw x8,  5 * 4(sp)
    lw x9,  6 * 4(sp)
    lw x10, 7 * 4(sp)
    lw x11, 8 * 4(sp)
    lw x12, 9 * 4(sp)
    lw x13, 10 * 4(sp)
    lw x14, 11 * 4(sp)
    lw x15, 12 * 4(sp)
    lw x16, 13 * 4(sp)
    lw x17, 14 * 4(sp)
    lw x18, 15 * 4(sp)
    lw x19, 16 * 4(sp)
    lw x20, 17 * 4(sp)
    lw x21, 18 * 4(sp)
    lw x22, 19 * 4(sp)
    lw x23, 20 * 4(sp)
    lw x24, 21 * 4(sp)
    lw x25, 22 * 4(sp)
    lw x26, 23 * 4(sp)
    lw x27, 24 * 4(sp)
    lw x28, 25 * 4(sp)
    lw x29, 26 * 4(sp)
    lw x30, 27 * 4(sp)
    lw x31, 28 * 4(sp)
    lw x4,  29 * 4(sp)

    addi sp, sp, portCONTEXT_SIZE
    mret
.endfunc

.section .text.xPortStartFirstTask,"ax",@progbits
.align 2
.func
xPortStartFirstTask:
    la sp, pxCurrentTCB
    lw sp, 0(sp)
    lw sp, 0(sp)

    lw x1,  0(sp)
    lw x6,  3 * 4(sp)
    lw x7,  4 * 4(sp)
    lw x8,  5 * 4(sp)
    lw x9,  6 * 4(sp)
    lw x10, 7 * 4(sp)
    lw x11, 8 * 4(sp)
    lw x12, 9 * 4(sp)
    lw x13, 10 * 4(sp)
    lw x14, 11 * 4(sp)
    lw x15, 12 * 4(sp)
    lw x16, 13 * 4(sp)
    lw x17, 14 * 4(sp)
    lw x18, 15 * 4(sp)
    lw x19, 16 * 4(sp)
    lw x20, 17 * 4(sp)
    lw x21, 18 * 4(sp)
    lw x22, 19 * 4(sp)
    lw x23, 20 * 4(sp)
    lw x24, 21 * 4(sp)
    lw x25, 22 * 4(sp)
    lw x26, 23 * 4(sp)
    lw x27, 24 * 4(sp)
    lw x28, 25 * 4(sp)
    lw x29, 26 * 4(sp)
    lw x30, 27 * 4(sp)
    lw x31, 28 * 4(sp)

    lw x5, 29 * 4(sp)
    ori x5, x5, 0x08
    csrw mstatus, x5
    lw x5, 2 * 4(sp)

    addi sp, sp, portCONTEXT_SIZE
    ret
.endfunc

.section .text.pxPortInitialiseStack,"ax",@progbits
.align 2
.func
pxPortInitialiseStack:
    csrr t0, mstatus
    andi t0, t0, ~0x8
    li t1, 0x1880
    or t0, t0, t1

    addi a0, a0, -4
    sw t0, 0(a0)
    addi a0, a0, -(22 * 4)
    sw a2, 0(a0)
    addi a0, a0, -(6 * 4)
    sw zero, 0(a0)
    addi a0, a0, -4
    sw a1, 0(a0)
    ret
.endfunc
