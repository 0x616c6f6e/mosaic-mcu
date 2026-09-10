#ifndef CHIP_API_TIME_H
#define CHIP_API_TIME_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The time implementation owns the SoC SysTick interrupt. */
chip_status_t chip_time_init(void);
bool chip_time_is_initialized(void);
uint64_t chip_time_ticks(void);
uint64_t chip_time_micros(void);
uint32_t chip_time_millis(void);
void chip_delay_us(uint32_t delay_us);
void chip_delay_ms(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif
