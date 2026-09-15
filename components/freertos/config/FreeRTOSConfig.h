#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/** @brief Report an assertion failure and stop the firmware.
 * @param file Source file containing the failed assertion.
 * @param line Source line containing the failed assertion.
 */
void freertos_platform_assert(const char *file, int line);

#ifndef PLATFORM_FREERTOS_CPU_CLOCK_HZ
/** @brief Default CPU clock used for FreeRTOS scheduling, in Hz. */
#define PLATFORM_FREERTOS_CPU_CLOCK_HZ      UINT32_C(62400000)
#endif
#ifndef PLATFORM_FREERTOS_TICK_RATE_HZ
/** @brief Default scheduler tick frequency in Hz. */
#define PLATFORM_FREERTOS_TICK_RATE_HZ      1000U
#endif
#ifndef PLATFORM_FREERTOS_HEAP_SIZE
/** @brief Default FreeRTOS heap allocation in bytes. */
#define PLATFORM_FREERTOS_HEAP_SIZE         (8U * 1024U)
#endif

/** @brief Preemptive scheduler with 32-bit ticks and eight priority levels. */
#define configCPU_CLOCK_HZ                  PLATFORM_FREERTOS_CPU_CLOCK_HZ
#define configTICK_RATE_HZ                  PLATFORM_FREERTOS_TICK_RATE_HZ
#define configTICK_TYPE_WIDTH_IN_BITS       TICK_TYPE_WIDTH_32_BITS
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1
#define configUSE_TICKLESS_IDLE             0
#define configMAX_PRIORITIES                8
#define configMINIMAL_STACK_SIZE            128U
#define configMAX_TASK_NAME_LEN             16
#define configIDLE_SHOULD_YIELD             1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/** @brief Use both dynamic and static RTOS allocation within a fixed heap. */
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     1
#define configKERNEL_PROVIDED_STATIC_MEMORY 1
#define configTOTAL_HEAP_SIZE               PLATFORM_FREERTOS_HEAP_SIZE
#define configAPPLICATION_ALLOCATED_HEAP    0
#define configHEAP_CLEAR_MEMORY_ON_FREE     0

/** @brief Disable unneeded hooks and runtime statistics to save flash/RAM. */
#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0
#define configUSE_MALLOC_FAILED_HOOK        0
#define configCHECK_FOR_STACK_OVERFLOW      0
#define configUSE_TRACE_FACILITY            0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configGENERATE_RUN_TIME_STATS       0
#define configUSE_NEWLIB_REENTRANT          0

/** @brief Enable synchronization and task notifications; omit timers/events. */
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_QUEUE_SETS                0
#define configQUEUE_REGISTRY_SIZE           0
#define configUSE_TASK_NOTIFICATIONS        1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configUSE_TIMERS                    0
#define configUSE_EVENT_GROUPS              0
#define configUSE_STREAM_BUFFERS            0
#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     1

/** @brief Expose the task-control API used by the demo applications. */
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskPrioritySet            1

/** @brief CH585 port supplies its own tick source instead of RISC-V MTIME. */
#define configMTIME_BASE_ADDRESS            0
#define configMTIMECMP_BASE_ADDRESS         0

/** @brief Fail fast with the assertion source location. */
#define configASSERT(expression) \
    do { \
        if ((expression) == 0) { \
            freertos_platform_assert(__FILE__, __LINE__); \
        } \
    } while (0)

#endif
