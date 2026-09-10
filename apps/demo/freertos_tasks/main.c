#include <stdbool.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>

#include <chip_system.h>
#include <platform_log.h>

#include "demo_support.h"

#define BLINK_TASK_STACK_WORDS   160U
#define REPORT_TASK_STACK_WORDS  192U

static volatile uint32_t blink_count;

static uint32_t freertos_log_timestamp(void *context)
{
    TickType_t ticks = xTaskGetTickCount();

    (void)context;
    return (uint32_t)(((uint64_t)ticks * UINT64_C(1000)) /
                      (uint64_t)configTICK_RATE_HZ);
}

static void blink_task(void *context)
{
    TickType_t wake_time = xTaskGetTickCount();
    bool led_on = false;

    (void)context;
    for (;;) {
        led_on = !led_on;
        (void)demo_led_write(led_on);
        ++blink_count;
        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(250U));
    }
}

static void report_task(void *context)
{
    TickType_t wake_time = xTaskGetTickCount();

    (void)context;
    for (;;) {
        LOG_INFO("rtos", "tick=%lu blink_count=%lu",
                 (unsigned long)xTaskGetTickCount(),
                 (unsigned long)blink_count);
        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    chip_system_config_t system_config = {
        .source = CHIP_CLOCK_INTERNAL,
        .core_clock_hz = DEMO_SYSTEM_CLOCK_HZ,
        .external_crystal_load_pf = 0U,
    };

    DEMO_REQUIRE(chip_system_init(&system_config));
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init_with_timestamp(freertos_log_timestamp, NULL));
    LOG_INFO("rtos", "FreeRTOS scheduler starting");

    if (xTaskCreate(blink_task, "blink", BLINK_TASK_STACK_WORDS, NULL, 2U,
                    NULL) != pdPASS) {
        LOG_ERROR("rtos", "failed to create blink task");
        demo_halt();
    }
    if (xTaskCreate(report_task, "report", REPORT_TASK_STACK_WORDS, NULL, 1U,
                    NULL) != pdPASS) {
        LOG_ERROR("rtos", "failed to create report task");
        demo_halt();
    }

    vTaskStartScheduler();
    LOG_ERROR("rtos", "scheduler returned unexpectedly");
    demo_halt();
}
