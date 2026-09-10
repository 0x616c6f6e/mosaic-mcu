#ifndef PORTMACRO_H
#define PORTMACRO_H

/*
 * Based on the WCH CH585 FreeRTOS V10.5.1 QingKe port.
 * Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define portSTACK_TYPE       uint32_t
#define portBASE_TYPE        int32_t
#define portUBASE_TYPE       uint32_t
#define portMAX_DELAY        ((TickType_t)UINT32_MAX)
#define portPOINTER_SIZE_TYPE uint32_t

typedef portSTACK_TYPE StackType_t;
typedef portBASE_TYPE BaseType_t;
typedef portUBASE_TYPE UBaseType_t;
typedef portUBASE_TYPE TickType_t;

#define portCHAR             char
#define portFLOAT            float
#define portDOUBLE           double
#define portLONG             long
#define portSHORT            short
#define portTICK_TYPE_IS_ATOMIC 1

#define portSTACK_GROWTH     (-1)
#define portTICK_PERIOD_MS   ((TickType_t)1000U / configTICK_RATE_HZ)
#define portBYTE_ALIGNMENT   16

void vPortYield(void);
#define portYIELD() vPortYield()
#define portEND_SWITCHING_ISR(switch_required) \
    do { \
        if ((switch_required) != pdFALSE) { \
            vPortYield(); \
        } \
    } while (0)
#define portYIELD_FROM_ISR(switch_required) \
    portEND_SWITCHING_ISR(switch_required)

#define portHAS_NESTED_INTERRUPTS 0
#define portCRITICAL_NESTING_IN_TCB 1
#define portDISABLE_INTERRUPTS() \
    do { \
        __asm volatile("csrc mstatus, 8" ::: "memory"); \
        __asm volatile("nop\nnop\nnop"); \
    } while (0)
#define portENABLE_INTERRUPTS() \
    __asm volatile("csrs mstatus, 8" ::: "memory")

void vTaskEnterCritical(void);
void vTaskExitCritical(void);
#define portENTER_CRITICAL() vTaskEnterCritical()
#define portEXIT_CRITICAL()  vTaskExitCritical()

#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
#if configMAX_PRIORITIES > 32
#error "Optimised task selection supports at most 32 priorities"
#endif
#define portRECORD_READY_PRIORITY(priority, ready_priorities) \
    ((ready_priorities) |= (UINT32_C(1) << (priority)))
#define portRESET_READY_PRIORITY(priority, ready_priorities) \
    ((ready_priorities) &= ~(UINT32_C(1) << (priority)))
#define portGET_HIGHEST_PRIORITY(top_priority, ready_priorities) \
    ((top_priority) = (UBaseType_t)(31U - \
        (UBaseType_t)__builtin_clz((unsigned int)(ready_priorities))))
#endif

#define portTASK_FUNCTION_PROTO(function, parameters) \
    void function(void *parameters)
#define portTASK_FUNCTION(function, parameters) \
    void function(void *parameters)
#define portNOP() __asm volatile("nop")
#define portINLINE inline
#define portFORCE_INLINE inline __attribute__((always_inline))
#define portMEMORY_BARRIER() __asm volatile("" ::: "memory")

#ifdef __cplusplus
}
#endif

#endif
