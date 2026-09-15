# SPI Flash

`platform_spi_flash` 是基于 Chip SPI/GPIO/Time API 的同步 SPI NOR Flash
组件，适用于使用标准 JEDEC 指令的 25 系列器件。组件支持：

- 读取 3 字节 JEDEC ID 和状态寄存器 1。
- 普通读取（`0x03`）。
- 自动跨页拆分的 Page Program（`0x02`）。
- 扇区擦除（`0x20`）和整片擦除（`0xC7`）。
- WIP 轮询、传输超时和操作超时。
- 可在初始化时通过 JEDEC 容量码自动识别容量。

组件在 `spi_flash_init()` 中将 CS 配置为高电平，并初始化指定 SPI 为 Mode 0、MSB
first。它因此拥有这一路 SPI 和 CS，调用 `spi_flash_deinit()` 时也会释放两者；共享总线的
应用应在外层串行化访问，且不要在设备存活期间重新配置 SPI。

```c
#include <spi_flash.h>

spi_flash_t flash;
const spi_flash_config_t config = {
    .spi = CHIP_SPI_0,
    .cs_pin = CHIP_PIN(CHIP_GPIO_PORT_A, 12),
    .clock_hz = UINT32_C(8000000),
    .capacity_bytes = 0U, /* 通过 JEDEC ID 自动识别 */
    .page_size = 256U,
    .sector_size = 4096U,
    .transfer_timeout_us = UINT32_C(100000),
    .program_timeout_us = UINT32_C(1000000),
    .sector_erase_timeout_us = UINT32_C(3000000),
    .chip_erase_timeout_us = UINT32_C(200000000),
};

chip_time_init();
spi_flash_init(&flash, &config);
```

编程只能把 bit 从 1 改成 0，调用者必须按器件要求先擦除。扇区擦除地址必须按
`sector_size` 对齐。有限的编程/擦除超时依赖已初始化的 Chip Time；纯读取不依赖计时器。
`capacity_bytes` 为 0 时，`spi_flash_init()` 会读取 JEDEC ID 并将识别出的字节数写回
`flash.config.capacity_bytes`；也可以填写非零值继续使用固定容量。

当前实现发送 3 字节地址，因此容量上限为 16 MiB，不支持 4-byte address、Fast Read、
Quad SPI、Suspend/Resume 或厂商专有状态寄存器。
