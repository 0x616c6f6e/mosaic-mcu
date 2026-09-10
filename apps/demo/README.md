# CH585 Demos

这些程序只通过 `chip_*.h` 使用 CH585 port，用于在开发板上验证平台接口。每个 demo 都是
独立固件，不共享运行时状态。

## 程序

| Target | 功能 | 预期现象 |
| --- | --- | --- |
| `ch585_demo_blinky` | System、Time、GPIO | PB8 每 500 ms 翻转 |
| `ch585_demo_uart_echo` | UART0 轮询收发 | 115200 8N1 回显输入字节 |
| `ch585_demo_gpio_irq` | GPIO 中断和 UART | PA8 下降沿翻转 PB8 并输出计数 |
| `ch585_demo_i2c_scan` | I2C 主机探测 | 每 2 秒通过 UART 输出 0x08-0x77 的响应地址 |
| `ch585_demo_spi_flash` | SPI NOR Flash | 输出 JEDEC ID，并验证末尾扇区的擦除、写入和回读 |
| `ch585_demo_spi_flash_littlefs` | SPI NOR + littlefs | 挂载或格式化文件系统，并持久化启动计数 |
| `ch585_demo_spi_flash_fatfs` | SPI NOR + FatFs | 挂载或格式化 FAT 卷，并持久化启动计数 |
| `ch585_demo_usb_hid` | USBFS Device | 枚举为 Boot Keyboard，周期发送按下/释放 `A` |
| `ch585_demo_tinyusb_hid` | TinyUSB + USBFS Device | 由 TinyUSB 枚举并周期发送按下/释放 `A` |
| `ch585_demo_ble_peripheral` | BLE Peripheral + UART | 广播为 `CH585 Demo`，允许连接并输出连接状态 |
| `ch585_demo_freertos_tasks` | FreeRTOS 任务调度 | PB8 每 250 ms 翻转，UART 每秒输出 tick 和计数 |
| `ch585_demo_freertos_usb_ble` | FreeRTOS + TinyUSB + BLE | USB HID、BLE 广播和三个任务并行运行 |
| `ch585_demo_json_parse` | jsmn JSON tokenizer | 解析 LED 配置、输出字段并按配置周期翻转 PB8 |

USB HID 中的 VID/PID 仅用于本地开发验证，不得直接用于正式产品发布。

## 默认连接

| 功能 | 引脚 |
| --- | --- |
| LED | PB8，高电平有效 |
| Button | PA8，内部上拉，下降沿触发 |
| UART0 TX | PB7 |
| UART0 RX | PB4 |
| I2C SCL | PB13 |
| I2C SDA | PB12 |
| SPI Flash CS | PA3 |
| SPI1 SCK | PA0 |
| SPI1 MOSI | PA1 |
| SPI1 MISO | PA2 |

不同开发板在烧录前应修改 `common/demo_board.h` 或通过编译宏覆盖这些值。I2C 建议使用外部
上拉电阻。GPIO IRQ demo 没有实现按键消抖，因此一次操作可能产生多次计数。

SPI Flash demo 默认按 2 MiB 容量、256 字节页和 4 KiB 扇区配置。它会擦除并改写最后一个
扇区，已有数据将丢失；写测试前会核对 JEDEC 容量码。器件参数或测试地址不同时必须覆盖
对应的 `DEMO_SPI_FLASH_*` 宏。
只读取 JEDEC ID 时可设置 `DEMO_SPI_FLASH_ENABLE_WRITE_TEST=0`。

SPI Flash + littlefs demo 默认将整颗 Flash 用作文件系统。首次运行会格式化 Flash，已有数据
将丢失；后续启动会挂载已有文件系统，更新 `boot_count` 文件并通过 UART 输出计数。分区范围
可通过 `DEMO_LITTLEFS_OFFSET_BYTES` 和 `DEMO_LITTLEFS_SIZE_BYTES` 调整。

SPI Flash + FatFs demo 默认也会占用整颗 Flash，首次运行会格式化为 FAT 卷，后续更新
`BOOTCNT.BIN`。分区范围可通过 `DEMO_FATFS_OFFSET_BYTES` 和 `DEMO_FATFS_SIZE_BYTES` 调整。
裸 NOR 上的 512 字节扇区写入通过 4 KiB 读改擦写实现，仅适合验证；它没有 FTL、磨损均衡
或掉电原子性，产品存储应优先使用 littlefs 或增加专用 FTL。

## 构建

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

构建目录的 `apps/demo/` 下会为每个 demo 生成 `.elf`、`.bin` 和 `.hex`。只构建某个
程序时使用：

```sh
cmake --build build --target ch585_demo_blinky
cmake --build build --target ch585_demo_spi_flash
cmake --build build --target ch585_demo_spi_flash_littlefs
cmake --build build --target ch585_demo_spi_flash_fatfs
cmake --build build --target ch585_demo_freertos_tasks
cmake --build build --target ch585_demo_freertos_usb_ble
```

BLE demo 使用原厂 Peripheral-only 协议栈和外部 32 MHz 高速晶振（默认负载电容 18 pF）；
主循环持续运行 TMOS，不执行阻塞延时。其 UART0 日志为 115200 8N1，可使用手机 BLE 扫描
工具发现并连接 `CH585 Demo`。

BLE demo 同时演示 `platform_log`，日志格式为 `[毫秒] 级别/tag: 内容`。当前 UART backend
为同步阻塞输出，只应在主循环或 TMOS 任务上下文使用，不应在硬件中断中调用。

组合 demo 的 USB 枚举为 `CH585 RTOS USB BLE` HID Keyboard，同时以 `CH585 Combo` 广播为
BLE HID Keyboard；BLE 连接并启用通知后，每 500 ms 交替发送 `A` 按下/释放。UART 每秒输出
USB 初始化、挂载、BLE 和 HID 通知状态。该 demo 使用实验性 FreeRTOS/BLE 调度桥接，需在
开发板上持续验证 USB 传输、BLE 连接和 RTOS tick 是否稳定。

不需要 demo 时可在配置阶段指定 `-DPLATFORM_BUILD_DEMOS=OFF`。

## 烧录

让目标进入 USB ISP 模式后，使用对应 target 构建并烧录，例如：

```sh
cmake --build build --target wchisp_info
cmake --build build --target flash_ch585_demo_blinky
cmake --build build --target flash_ch585_demo_spi_flash
cmake --build build --target flash_ch585_demo_spi_flash_littlefs
cmake --build build --target flash_ch585_demo_spi_flash_fatfs
cmake --build build --target flash_ch585_demo_ble_peripheral
cmake --build build --target flash_ch585_demo_freertos_tasks
cmake --build build --target flash_ch585_demo_freertos_usb_ble
```

详细的 USB 权限、设备选择和 UART ISP 配置见[烧录文档](../../docs/flashing-with-wchisp.md)。
