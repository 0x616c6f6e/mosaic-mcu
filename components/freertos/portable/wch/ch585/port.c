/*
 * Based on the WCH CH585 FreeRTOS V10.5.1 QingKe port.
 * Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "FreeRTOS.h"
#include "task.h"

#include "CH58x_common.h"

extern void xPortStartFirstTask(void);
extern void SW_Handler(void);
extern void SysTick_Handler(void);

void vPortYield(void)
{
    PFIC_SetPendingIRQ(SWI_IRQn);
}

__attribute__((section(".highcode")))
void vPortBleSchedulerSuspend(void)
{
    PFIC_DisableIRQ(SysTick_IRQn);
    PFIC_DisableIRQ(SWI_IRQn);
}

__attribute__((section(".highcode")))
void vPortBleSchedulerResume(void)
{
    PFIC_EnableIRQ(SWI_IRQn);
    PFIC_EnableIRQ(SysTick_IRQn);
}

void vPortSetupTimerInterrupt(void)
{
    SetVTFIRQ((uint32_t)SW_Handler, SWI_IRQn, 0U, ENABLE);
    SetVTFIRQ((uint32_t)SysTick_Handler, SysTick_IRQn, 1U, ENABLE);

    PFIC_SetPriority(SWI_IRQn, UINT8_C(0xF0));
    PFIC_SetPriority(SysTick_IRQn, UINT8_C(0xF0));
    PFIC_ClearPendingIRQ(SWI_IRQn);
    PFIC_ClearPendingIRQ(SysTick_IRQn);
    PFIC_EnableIRQ(SWI_IRQn);

    configASSERT(SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ) == 0U);
}

BaseType_t xPortStartScheduler(void)
{
    uint32_t vector_mode;

    __asm volatile("csrr %0, mtvec" : "=r"(vector_mode));
    configASSERT((vector_mode & UINT32_C(0x03)) == UINT32_C(0x03));

    vPortSetupTimerInterrupt();
    xPortStartFirstTask();
    return pdFAIL;
}

void vPortEndScheduler(void)
{
    portDISABLE_INTERRUPTS();
    for (;;) {
    }
}

__attribute__((section(".highcode")))
void ch585_systick_hook(void)
{
    if (xTaskIncrementTick() != pdFALSE) {
        vPortYield();
    }
}

void freertos_platform_assert(const char *file, int line)
{
    (void)file;
    (void)line;
    portDISABLE_INTERRUPTS();
    for (;;) {
    }
}
