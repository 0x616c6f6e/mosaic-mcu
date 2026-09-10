#ifndef CH585_DEMO_BOARD_H
#define CH585_DEMO_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_i2c.h>
#include <chip_uart.h>

#ifndef DEMO_SYSTEM_CLOCK_HZ
#define DEMO_SYSTEM_CLOCK_HZ UINT32_C(62400000)
#endif

#ifndef DEMO_LED_PIN
#define DEMO_LED_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 8)
#endif

#ifndef DEMO_LED_ACTIVE_HIGH
#define DEMO_LED_ACTIVE_HIGH true
#endif

#ifndef DEMO_BUTTON_PIN
#define DEMO_BUTTON_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 8)
#endif

#ifndef DEMO_UART_INSTANCE
#define DEMO_UART_INSTANCE CHIP_UART_0
#endif

#ifndef DEMO_UART_BAUD_RATE
#define DEMO_UART_BAUD_RATE UINT32_C(115200)
#endif

/* UART0 default route: TX=PB7, RX=PB4. */
#ifndef DEMO_UART_TX_PIN
#define DEMO_UART_TX_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 7)
#endif

#ifndef DEMO_UART_RX_PIN
#define DEMO_UART_RX_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 4)
#endif

/* I2C default route: SCL=PB13, SDA=PB12. External pull-ups are recommended. */
#ifndef DEMO_I2C_INSTANCE
#define DEMO_I2C_INSTANCE CHIP_I2C_0
#endif

#ifndef DEMO_I2C_SCL_PIN
#define DEMO_I2C_SCL_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 13)
#endif

#ifndef DEMO_I2C_SDA_PIN
#define DEMO_I2C_SDA_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 12)
#endif

#endif
