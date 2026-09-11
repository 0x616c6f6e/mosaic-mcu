add_ch585_demo(spi_flash_fatfs
    SOURCES main.c
    LIBRARIES
        platform::fatfs
        platform::spi_flash
)
