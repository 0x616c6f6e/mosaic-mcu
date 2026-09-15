#ifndef CH585_DEMO_BOARD_H
#define CH585_DEMO_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_i2c.h>
#include <chip_spi.h>
#include <chip_uart.h>

#ifndef DEMO_SYSTEM_CLOCK_HZ
/** @brief Demo board core clock in Hz. */
#define DEMO_SYSTEM_CLOCK_HZ UINT32_C(62400000)
#endif

#ifndef DEMO_LED_PIN
/** @brief Status LED GPIO. */
#define DEMO_LED_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 8)
#endif

#ifndef DEMO_LED_ACTIVE_HIGH
/** @brief true if a high GPIO level turns on the status LED. */
#define DEMO_LED_ACTIVE_HIGH true
#endif

#ifndef DEMO_BUTTON_PIN
/** @brief Demo user button GPIO. */
#define DEMO_BUTTON_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 8)
#endif

#ifndef DEMO_UART_INSTANCE
/** @brief Serial console UART instance. */
#define DEMO_UART_INSTANCE CHIP_UART_0
#endif

#ifndef DEMO_UART_BAUD_RATE
/** @brief Serial console rate in bits per second. */
#define DEMO_UART_BAUD_RATE UINT32_C(115200)
#endif

/** @brief Default UART0 TX route: PB7. */
#ifndef DEMO_UART_TX_PIN
#define DEMO_UART_TX_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 7)
#endif

/** @brief Default UART0 RX route: PB4. */
#ifndef DEMO_UART_RX_PIN
#define DEMO_UART_RX_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 4)
#endif

/** @brief Default I2C controller; external pull-ups are recommended. */
#ifndef DEMO_I2C_INSTANCE
#define DEMO_I2C_INSTANCE CHIP_I2C_0
#endif

/** @brief Default I2C SCL route: PB13. */
#ifndef DEMO_I2C_SCL_PIN
#define DEMO_I2C_SCL_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 13)
#endif

/** @brief Default I2C SDA route: PB12. */
#ifndef DEMO_I2C_SDA_PIN
#define DEMO_I2C_SDA_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 12)
#endif

/** @brief SPI NOR controller used by the demo board. */
#ifndef DEMO_SPI_FLASH_INSTANCE
#define DEMO_SPI_FLASH_INSTANCE CHIP_SPI_1
#endif

/** @brief SPI NOR CS route: PA3. */
#ifndef DEMO_SPI_FLASH_CS_PIN
#define DEMO_SPI_FLASH_CS_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 3)
#endif

/** @brief SPI NOR SCK route: PA0. */
#ifndef DEMO_SPI_FLASH_SCK_PIN
#define DEMO_SPI_FLASH_SCK_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 0)
#endif

/** @brief SPI NOR MOSI route: PA1. */
#ifndef DEMO_SPI_FLASH_MOSI_PIN
#define DEMO_SPI_FLASH_MOSI_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 1)
#endif

/** @brief SPI NOR MISO route: PA2. */
#ifndef DEMO_SPI_FLASH_MISO_PIN
#define DEMO_SPI_FLASH_MISO_PIN CHIP_PIN(CHIP_GPIO_PORT_A, 2)
#endif

/** @brief Default SPI NOR clock in Hz. */
#ifndef DEMO_SPI_FLASH_CLOCK_HZ
#define DEMO_SPI_FLASH_CLOCK_HZ UINT32_C(500000)
#endif

/** @brief SPI NOR capacity used by storage demos in bytes. */
#ifndef DEMO_SPI_FLASH_CAPACITY_BYTES
#define DEMO_SPI_FLASH_CAPACITY_BYTES \
    (UINT32_C(8) * UINT32_C(1024) * UINT32_C(1024))
#endif

/** @brief SPI NOR program page size in bytes. */
#ifndef DEMO_SPI_FLASH_PAGE_SIZE
#define DEMO_SPI_FLASH_PAGE_SIZE UINT32_C(256)
#endif

/** @brief SPI NOR erase sector size in bytes. */
#ifndef DEMO_SPI_FLASH_SECTOR_SIZE
#define DEMO_SPI_FLASH_SECTOR_SIZE UINT32_C(4096)
#endif

/** @brief Enable the demo's destructive SPI NOR write/erase test. */
#ifndef DEMO_SPI_FLASH_ENABLE_WRITE_TEST
#define DEMO_SPI_FLASH_ENABLE_WRITE_TEST 1
#endif

/** @brief Flash address of the sector used by the write/erase test. */
#ifndef DEMO_SPI_FLASH_TEST_ADDRESS
#define DEMO_SPI_FLASH_TEST_ADDRESS \
    (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_SPI_FLASH_SECTOR_SIZE)
#endif

/** @brief SPI transfer timeout in microseconds. */
#ifndef DEMO_SPI_FLASH_TRANSFER_TIMEOUT_US
#define DEMO_SPI_FLASH_TRANSFER_TIMEOUT_US UINT32_C(100000)
#endif

/** @brief Page program timeout in microseconds. */
#ifndef DEMO_SPI_FLASH_PROGRAM_TIMEOUT_US
#define DEMO_SPI_FLASH_PROGRAM_TIMEOUT_US UINT32_C(1000000)
#endif

/** @brief Sector erase timeout in microseconds. */
#ifndef DEMO_SPI_FLASH_ERASE_TIMEOUT_US
#define DEMO_SPI_FLASH_ERASE_TIMEOUT_US UINT32_C(3000000)
#endif

/** @brief Whole-chip erase timeout in microseconds. */
#ifndef DEMO_SPI_FLASH_CHIP_ERASE_TIMEOUT_US
#define DEMO_SPI_FLASH_CHIP_ERASE_TIMEOUT_US UINT32_C(200000000)
#endif

/** @brief LittleFS partition start address in flash bytes. */
#ifndef DEMO_LITTLEFS_OFFSET_BYTES
#define DEMO_LITTLEFS_OFFSET_BYTES UINT32_C(0)
#endif

/** @brief LittleFS partition length in bytes. */
#ifndef DEMO_LITTLEFS_SIZE_BYTES
#define DEMO_LITTLEFS_SIZE_BYTES \
    (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_LITTLEFS_OFFSET_BYTES)
#endif

/** @brief FAT partition start address in flash bytes. */
#ifndef DEMO_FATFS_OFFSET_BYTES
#define DEMO_FATFS_OFFSET_BYTES UINT32_C(0)
#endif

/** @brief FAT partition length in bytes. */
#ifndef DEMO_FATFS_SIZE_BYTES
#define DEMO_FATFS_SIZE_BYTES \
    (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_FATFS_OFFSET_BYTES)
#endif

#endif
