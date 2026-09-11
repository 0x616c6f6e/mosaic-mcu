add_ch585_demo(spi_flash_littlefs
    SOURCES main.c
    LIBRARIES
        platform::littlefs
        platform::spi_flash
)
