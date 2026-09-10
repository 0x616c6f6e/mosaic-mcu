# mosaic-mcu 架构与移植规范

> 状态：第一版
>
> 当前参考芯片：WCH CH585

## 1. 平台目标

本项目先搭建通用 MCU 平台，再在平台之上承载键盘、传感器节点或其他应用。平台提供：

- 厂商 SDK 的版本化管理和构建隔离。
- 每颗 SoC 对一致 Chip API 的实现。
- 面向应用和通用组件的一致 Chip API。
- 板级资源描述、工具链、调试和契约测试基础设施。

平台层不感知任何具体应用，不允许键盘业务成为底层接口设计的前提。

## 2. 架构分层

```text
apps/                      产品与场景代码
    |
components/                跨应用可复用组件
    |
chips/<vendor>/<soc>/port/include/
                           当前芯片提供的 Chip API
    |
chips/<vendor>/<soc>/port/src/
                           厂商类型转换和硬件实现
    |
chips/<vendor>/<soc>/sdk/  原厂 SDK
```

依赖只能自上而下。中断回调是反向控制流，但回调签名必须由 `chip_*.h` 定义，不能透传
厂商事件结构。

## 3. 当前 Chip API

CH585 port 当前公开：

| 头文件 | 能力 |
| --- | --- |
| `<chip_status.h>` | 统一状态码和超时常量 |
| `<chip_system.h>` | 系统时钟、芯片 ID、唯一 ID、临界区和复位 |
| `<chip_time.h>` | 单调时间、微秒/毫秒时间和延时 |
| `<chip_gpio.h>` | GPIO 配置、读写和中断回调 |
| `<chip_uart.h>` | UART0-3 轮询收发 |
| `<chip_spi.h>` | SPI0-1 主机全双工传输 |
| `<chip_i2c.h>` | I2C 主机和组合写读 |
| `<chip_adc.h>` | 外部 ADC、电池和温度通道 |
| `<chip_flash.h>` | Data-Flash 擦写和读取 |
| `<chip_pwm.h>` | PWM4-11 |
| `<chip_power.h>` | DCDC 与 idle |
| `<chip_usb_device.h>` | USBFS Device、EP0-EP7 和异步事件 |
| `<chip_ble.h>` | BLE Peripheral、广播控制和连接状态 |
| `<chip_ble_hid_keyboard.h>` | BLE HID Keyboard 输入报告和 LED 输出 |

接口约定：

- 可失败的操作返回 `chip_status_t`。
- 超时统一使用微秒，`CHIP_TIMEOUT_NONE` 表示不等待，`CHIP_TIMEOUT_FOREVER` 表示永久等待。
- 参数使用定宽整数和平台枚举，不使用厂商枚举。
- 不支持的硬件能力返回 `CHIP_ERROR_UNSUPPORTED`，不能静默成功。
- GPIO 回调运行在中断上下文，不得阻塞。

## 4. CH585 初始化顺序

```text
原厂启动文件和 C 运行时
  -> chip_system_init()
  -> board_init()
  -> chip_time_init()
  -> 通用组件初始化
  -> 应用初始化
```

`chip_time_init()` 使用并独占 SysTick。有限超时依赖时间模块，因此 UART、SPI、I2C、ADC 的
有限超时操作必须在时间初始化之后调用。时间初始化后不得再改变系统时钟。

外设引脚和 `GPIOPinRemap` 由板级代码配置，chip port 不决定使用哪组 UART、SPI、I2C 或
PWM 复用引脚。

## 5. SDK 管理

- 原厂内容固定在 `chips/wch/ch585/sdk/`，自研代码固定在 `port/`。
- SDK 文件不进行格式化和无目的重构。
- 必须修改 SDK 时保存独立补丁，并在 `SDK.md` 记录原因。
- 厂商 include 通过 CMake `PRIVATE` 依赖限制在 port 内部。
- 预编译库必须进行架构兼容和完整链接验证。

当前 SDK 快照由不同时间的文件组成，发行包版本和许可证仍需补全后才能对外分发。

通用日志位于 `components/log`，通过 backend 回调接入 UART、USB CDC 等输出，不属于 Chip
API。应用链接 `platform::log` 后使用 `LOG_DEBUG/INFO/WARN/ERROR`，日志组件不得直接包含
厂商 SDK 头文件。

## 6. 构建模型

```text
application ELF
├── board target
├── reusable components
└── chip::selected
    ├── chip::api
    ├── wch_ch585_sdk
    └── libISP585.a
```

CH585 使用仓库内 `tools/wch/Toolchain/RISC-V Embedded GCC12`。配置命令：

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake
cmake --build build
```

应用 CMake：

```cmake
add_executable(example_app src/main.c)
configure_ch585_executable(example_app)
```

`configure_ch585_executable` 添加 CH585 启动文件、链接脚本、架构选项和选中 port。

FreeRTOS 应用额外链接 `platform::freertos`。CH585 port 使用 SysTick 产生 1 kHz tick，使用
PFIC SWI 执行上下文切换；因此 FreeRTOS 应用不得调用 `chip_time_init()`。

## 7. 中断和并发

- ISR 名称和硬件标志处理留在芯片 port。
- ISR 只做确认、数据搬运和回调，不执行复杂业务。
- 共享状态使用临界区或原子操作，不能只依赖 `volatile`。
- CH585 的 PB8/PB9 与 PB22/PB23 共享中断映射，冲突注册返回 `CHIP_ERROR_BUSY`。
- FreeRTOS 接管 SysTick，通过 CH585 专用 hook 与 Chip Time handler 分离；当前仅支持单核、
  非嵌套中断模式。

## 8. 错误和边界保护

- 初始化参数错误返回 `CHIP_ERROR_INVALID_ARG`。
- 未初始化外设返回 `CHIP_ERROR_NOT_READY`。
- 外设忙、超时和 I/O 错误分别返回明确状态。
- Data-Flash API 使用零基偏移并检查 32 KiB 边界、擦除对齐和缓冲区对齐要求。
- PWM 通道共享频率，不允许活动通道配置互不兼容的频率。

## 9. 测试要求

主机侧：

- 每个 `chip_*.h` 头文件可以被 C11 和 C++17 独立包含。
- 公共头文件中搜索不到 `CH58x_`、寄存器宏或厂商类型。
- 组件通过 fake port 测试错误传播、超时和状态机。

目标侧：

- 所有 port 和使用到的 SDK 文件以 WCH 编译器零告警构建。
- 启动文件、链接脚本、port、SDK 与 `libISP585.a` 可以组成无未解析符号的 ELF。
- 每颗开发板执行 GPIO、时间、UART、SPI、I2C、ADC、Flash、PWM 和 USBFS 冒烟测试。

## 10. 应用边界

键盘应用放在 `apps/keyboard/`，其中可以包含矩阵扫描、消抖、键位映射、HID 报告、USB
设备协议和 BLE HID。平台只为其提供通用硬件能力，不把这些概念加入 Chip API。

USBFS Device 控制器已经由 `chip_usb_device` 独立封装，不依赖原厂固定端点示例。描述符、
标准请求、HID/CDC 等类驱动仍属于上层 USB 协议栈。CH585 的 `USB2` 高速控制器尚未纳入
当前 port；BLE 当前仅覆盖 Peripheral 基础生命周期和广播控制。

## 11. 下一阶段

1. 增加实际 CH585 开发板目录和引脚复用配置。
2. 建立 `tests/contract/`，约束未来 SoC port 的源码兼容性。
3. 按真实复用需求建立 `components/` 模块。
4. 完成开发板冒烟测试。
5. 创建 `apps/keyboard/`，在平台稳定接口之上实现键盘功能。
