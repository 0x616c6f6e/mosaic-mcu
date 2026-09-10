# 使用 wchisp 烧录固件

项目使用 [ch32-rs/wchisp](https://github.com/ch32-rs/wchisp) 通过 USB ISP 或 UART ISP
烧录 WCH MCU。它与 WCH-Link/OpenOCD 是不同的传输方式。

## 1. 准备设备

1. 按开发板说明让 CH585 进入 Bootloader/ISP 模式。
2. USB 连接到用于 ISP 的接口。
3. 执行 `cmake --build build --target wchisp_probe` 检查设备。
4. 执行 `cmake --build build --target wchisp_info` 核对芯片型号、UID 和 Flash 容量。

烧录前必须确认 `wchisp_info` 识别出的目标确实是预期设备。多个设备同时连接时，通过
`-DWCHISP_DEVICE_INDEX=<index>` 选择设备。

Linux 普通用户没有 USB 访问权限时，可以配置上游建议的 udev 规则：

```udev
SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="55e0", MODE="0660", GROUP="plugdev", TAG+="uaccess"
```

项目已提供安装脚本：

```sh
tools/wch/wchisp/install-udev-rule.sh
```

也可以通过构建 target 安装：

```sh
cmake --build build --target wchisp_install_udev_rule
```

脚本会安装规则、重新加载 udev 并触发当前设备，无需依赖固定的 USB Bus/Device 编号。
不要通过长期使用 root 权限绕过设备权限。

## 2. 构建

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

仓库内默认使用 `tools/wch/wchisp/bin/wchisp`。也可以在配置阶段通过
`-DWCHISP_EXECUTABLE=/path/to/wchisp` 覆盖。

## 3. 烧录 Demo

```sh
cmake --build build --target flash_ch585_demo_blinky
cmake --build build --target flash_ch585_demo_uart_echo
cmake --build build --target flash_ch585_demo_gpio_irq
cmake --build build --target flash_ch585_demo_i2c_scan
cmake --build build --target flash_ch585_demo_usb_hid
cmake --build build --target flash_ch585_demo_tinyusb_hid
```

每个烧录 target 会先更新对应 ELF，然后执行 `wchisp flash <firmware.elf>`。默认流程会擦除
所需代码区、写入、校验并复位目标。只校验已有固件时使用对应的 `verify_<target>`。

## 4. UART ISP

```sh
cmake -S . -B build-uart -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DWCHISP_TRANSPORT=serial \
  -DWCHISP_SERIAL_PORT=/dev/ttyUSB0 \
  -DWCHISP_BAUDRATE=Baud115200
```

可选波特率与 `wchisp 0.3.0` CLI 一致：`Baud115200`、`Baud1m` 和 `Baud2m`。

## 5. 其他参数

| CMake 参数 | 用途 |
| --- | --- |
| `WCHISP_DEVICE_INDEX` | 多个 USB ISP 设备中的序号 |
| `WCHISP_RETRY_SECONDS` | 等待慢速 Bootloader 出现的秒数，默认 3 |
| `WCHISP_EXTRA_ARGS` | 传给 `wchisp` 的全局参数，例如 `--verbose` |
| `WCHISP_FLASH_ARGS` | 传给 `flash` 的参数，例如 `--no-reset` |

`--no-erase` 和 `--no-verify` 会降低默认保护措施，只应在明确理解目标 Flash 状态时使用。
