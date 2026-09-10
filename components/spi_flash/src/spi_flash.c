#include <spi_flash.h>

#include <string.h>

#include <chip_time.h>

#define SPI_FLASH_CMD_READ_JEDEC_ID UINT8_C(0x9F)
#define SPI_FLASH_CMD_READ_STATUS   UINT8_C(0x05)
#define SPI_FLASH_CMD_WRITE_ENABLE  UINT8_C(0x06)
#define SPI_FLASH_CMD_READ_DATA     UINT8_C(0x03)
#define SPI_FLASH_CMD_PAGE_PROGRAM  UINT8_C(0x02)
#define SPI_FLASH_CMD_SECTOR_ERASE  UINT8_C(0x20)
#define SPI_FLASH_CMD_CHIP_ERASE    UINT8_C(0xC7)

#define SPI_FLASH_STATUS_BUSY UINT8_C(0x01)
#define SPI_FLASH_POLL_INTERVAL_US UINT32_C(50)

static bool flash_config_valid(const spi_flash_config_t *config)
{
    return (config != NULL) &&
           (config->spi >= CHIP_SPI_0) && (config->spi < CHIP_SPI_COUNT) &&
           (config->clock_hz != 0U) &&
           (config->capacity_bytes != 0U) &&
           (config->capacity_bytes <= SPI_FLASH_MAX_CAPACITY_BYTES) &&
           (config->page_size != 0U) &&
           (config->page_size <= config->sector_size) &&
           (config->sector_size <= config->capacity_bytes) &&
           ((config->capacity_bytes % config->sector_size) == 0U);
}

static bool flash_ready(const spi_flash_t *flash)
{
    return (flash != NULL) && flash->initialized;
}

static bool flash_range_valid(const spi_flash_t *flash, uint32_t address,
                              size_t size)
{
    if (address > flash->config.capacity_bytes) {
        return false;
    }
    return size <= (size_t)(flash->config.capacity_bytes - address);
}

static void flash_address_command(uint8_t command[4], uint8_t opcode,
                                  uint32_t address)
{
    command[0] = opcode;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;
}

static chip_status_t flash_select(spi_flash_t *flash)
{
    return chip_gpio_write(flash->config.cs_pin, false);
}

static chip_status_t flash_deselect(spi_flash_t *flash,
                                    chip_status_t transaction_status)
{
    chip_status_t status = chip_gpio_write(flash->config.cs_pin, true);

    return (transaction_status != CHIP_OK) ? transaction_status : status;
}

static chip_status_t flash_transfer(spi_flash_t *flash,
                                    const uint8_t *tx_data,
                                    uint8_t *rx_data, size_t size)
{
    return chip_spi_transfer(flash->config.spi, tx_data, rx_data, size,
                             flash->config.transfer_timeout_us);
}

static chip_status_t flash_command(spi_flash_t *flash, const uint8_t *command,
                                   size_t command_size)
{
    chip_status_t status = flash_select(flash);

    if (status != CHIP_OK) {
        return status;
    }
    status = flash_transfer(flash, command, NULL, command_size);
    return flash_deselect(flash, status);
}

static chip_status_t flash_write_enable(spi_flash_t *flash)
{
    const uint8_t command = SPI_FLASH_CMD_WRITE_ENABLE;

    return flash_command(flash, &command, sizeof(command));
}

static chip_status_t flash_require_timer(uint32_t timeout_us)
{
    if ((timeout_us != CHIP_TIMEOUT_NONE) &&
        (timeout_us != CHIP_TIMEOUT_FOREVER) &&
        !chip_time_is_initialized()) {
        return CHIP_ERROR_NOT_READY;
    }
    return CHIP_OK;
}

chip_status_t spi_flash_init(spi_flash_t *flash,
                             const spi_flash_config_t *config)
{
    chip_gpio_config_t gpio_config = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_HIGH,
        .initial_level = true,
    };
    chip_spi_config_t spi_config;
    chip_status_t status;

    if ((flash == NULL) || !flash_config_valid(config)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    memset(flash, 0, sizeof(*flash));
    flash->config = *config;
    status = chip_gpio_init(config->cs_pin, &gpio_config);
    if (status != CHIP_OK) {
        return status;
    }

    spi_config.clock_hz = config->clock_hz;
    spi_config.mode = CHIP_SPI_MODE_0;
    spi_config.bit_order = CHIP_SPI_MSB_FIRST;
    status = chip_spi_init(config->spi, &spi_config);
    if (status != CHIP_OK) {
        (void)chip_gpio_deinit(config->cs_pin);
        return status;
    }

    flash->initialized = true;
    return CHIP_OK;
}

chip_status_t spi_flash_deinit(spi_flash_t *flash)
{
    chip_status_t first_status;
    chip_status_t status;

    if (!flash_ready(flash)) {
        return (flash == NULL) ? CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }

    first_status = chip_gpio_write(flash->config.cs_pin, true);
    status = chip_spi_deinit(flash->config.spi);
    if (first_status == CHIP_OK) {
        first_status = status;
    }
    status = chip_gpio_deinit(flash->config.cs_pin);
    if (first_status == CHIP_OK) {
        first_status = status;
    }
    flash->initialized = false;
    return first_status;
}

chip_status_t spi_flash_read_jedec_id(spi_flash_t *flash,
                                      spi_flash_jedec_id_t *id)
{
    const uint8_t command = SPI_FLASH_CMD_READ_JEDEC_ID;
    uint8_t response[3];
    chip_status_t status;

    if (!flash_ready(flash) || (id == NULL)) {
        return (flash == NULL || id == NULL) ? CHIP_ERROR_INVALID_ARG :
                                               CHIP_ERROR_NOT_READY;
    }

    status = flash_select(flash);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_transfer(flash, &command, NULL, sizeof(command));
    if (status == CHIP_OK) {
        status = flash_transfer(flash, NULL, response, sizeof(response));
    }
    status = flash_deselect(flash, status);
    if (status == CHIP_OK) {
        id->manufacturer = response[0];
        id->memory_type = response[1];
        id->capacity = response[2];
    }
    return status;
}

chip_status_t spi_flash_read_status(spi_flash_t *flash, uint8_t *status_byte)
{
    const uint8_t command = SPI_FLASH_CMD_READ_STATUS;
    chip_status_t status;

    if (!flash_ready(flash) || (status_byte == NULL)) {
        return (flash == NULL || status_byte == NULL) ? CHIP_ERROR_INVALID_ARG :
                                                        CHIP_ERROR_NOT_READY;
    }

    status = flash_select(flash);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_transfer(flash, &command, NULL, sizeof(command));
    if (status == CHIP_OK) {
        status = flash_transfer(flash, NULL, status_byte, 1U);
    }
    return flash_deselect(flash, status);
}

chip_status_t spi_flash_wait_ready(spi_flash_t *flash, uint32_t timeout_us)
{
    uint64_t started_at;
    chip_status_t status;

    if (!flash_ready(flash)) {
        return (flash == NULL) ? CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }
    status = flash_require_timer(timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    started_at = (timeout_us == CHIP_TIMEOUT_FOREVER) ? 0U :
                                                         chip_time_micros();

    for (;;) {
        uint8_t status_byte;

        status = spi_flash_read_status(flash, &status_byte);
        if (status != CHIP_OK) {
            return status;
        }
        if ((status_byte & SPI_FLASH_STATUS_BUSY) == 0U) {
            return CHIP_OK;
        }
        if ((timeout_us == CHIP_TIMEOUT_NONE) ||
            ((timeout_us != CHIP_TIMEOUT_FOREVER) &&
             ((chip_time_micros() - started_at) >= timeout_us))) {
            return CHIP_ERROR_TIMEOUT;
        }
        chip_delay_us(SPI_FLASH_POLL_INTERVAL_US);
    }
}

chip_status_t spi_flash_read(spi_flash_t *flash, uint32_t address,
                             void *data, size_t size)
{
    uint8_t command[4];
    chip_status_t status;

    if (!flash_ready(flash) || ((data == NULL) && (size != 0U))) {
        return (flash == NULL || ((data == NULL) && (size != 0U))) ?
               CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }
    if (!flash_range_valid(flash, address, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (size == 0U) {
        return CHIP_OK;
    }

    flash_address_command(command, SPI_FLASH_CMD_READ_DATA, address);
    status = flash_select(flash);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_transfer(flash, command, NULL, sizeof(command));
    if (status == CHIP_OK) {
        status = flash_transfer(flash, NULL, data, size);
    }
    return flash_deselect(flash, status);
}

chip_status_t spi_flash_program(spi_flash_t *flash, uint32_t address,
                                const void *data, size_t size)
{
    const uint8_t *source = data;
    chip_status_t status;

    if (!flash_ready(flash) || ((data == NULL) && (size != 0U))) {
        return (flash == NULL || ((data == NULL) && (size != 0U))) ?
               CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }
    if (!flash_range_valid(flash, address, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (size == 0U) {
        return CHIP_OK;
    }
    status = flash_require_timer(flash->config.program_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    status = spi_flash_wait_ready(flash, flash->config.program_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }

    while (size > 0U) {
        uint8_t command[4];
        size_t page_remaining = flash->config.page_size -
                                (address % flash->config.page_size);
        size_t chunk_size = (size < page_remaining) ? size : page_remaining;

        status = flash_write_enable(flash);
        if (status != CHIP_OK) {
            return status;
        }
        flash_address_command(command, SPI_FLASH_CMD_PAGE_PROGRAM, address);
        status = flash_select(flash);
        if (status != CHIP_OK) {
            return status;
        }
        status = flash_transfer(flash, command, NULL, sizeof(command));
        if (status == CHIP_OK) {
            status = flash_transfer(flash, source, NULL, chunk_size);
        }
        status = flash_deselect(flash, status);
        if (status != CHIP_OK) {
            return status;
        }
        status = spi_flash_wait_ready(flash,
                                      flash->config.program_timeout_us);
        if (status != CHIP_OK) {
            return status;
        }

        address += (uint32_t)chunk_size;
        source += chunk_size;
        size -= chunk_size;
    }
    return CHIP_OK;
}

chip_status_t spi_flash_erase_sector(spi_flash_t *flash, uint32_t address)
{
    uint8_t command[4];
    chip_status_t status;

    if (!flash_ready(flash)) {
        return (flash == NULL) ? CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }
    if ((address >= flash->config.capacity_bytes) ||
        ((address % flash->config.sector_size) != 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    status = flash_require_timer(flash->config.sector_erase_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    status = spi_flash_wait_ready(flash,
                                  flash->config.sector_erase_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_write_enable(flash);
    if (status != CHIP_OK) {
        return status;
    }
    flash_address_command(command, SPI_FLASH_CMD_SECTOR_ERASE, address);
    status = flash_command(flash, command, sizeof(command));
    if (status != CHIP_OK) {
        return status;
    }
    return spi_flash_wait_ready(flash,
                                flash->config.sector_erase_timeout_us);
}

chip_status_t spi_flash_erase_chip(spi_flash_t *flash)
{
    const uint8_t command = SPI_FLASH_CMD_CHIP_ERASE;
    chip_status_t status;

    if (!flash_ready(flash)) {
        return (flash == NULL) ? CHIP_ERROR_INVALID_ARG : CHIP_ERROR_NOT_READY;
    }
    status = flash_require_timer(flash->config.chip_erase_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    status = spi_flash_wait_ready(flash, flash->config.chip_erase_timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_write_enable(flash);
    if (status != CHIP_OK) {
        return status;
    }
    status = flash_command(flash, &command, sizeof(command));
    if (status != CHIP_OK) {
        return status;
    }
    return spi_flash_wait_ready(flash, flash->config.chip_erase_timeout_us);
}
