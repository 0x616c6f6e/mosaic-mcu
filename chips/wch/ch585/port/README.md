# CH585 Chip Port

本目录定义并实现 CH585 提供给上层组件和应用的通用 Chip API。公共头文件位于 `include/`，采用
`chip_*.h` 前缀，调用方使用 `<chip_gpio.h>`、`<chip_time.h>` 等包含路径。

- `src/`：厂商 API 转换、中断桥接和 CH585 具体实现。
- `include/`：提供给上层的 `chip_*.h` 公共头文件。
- `src/ch585_internal.h`：仅供本 port 使用的私有头文件。
- 可以包含 `CH58x_*.h`，但不得向公共头文件或上层组件暴露厂商类型。
- 不实现矩阵扫描、消抖、键位映射等键盘业务。

## 当前覆盖

| 模块 | 实现说明 |
| --- | --- |
| System | 时钟选择、芯片 ID、唯一 ID、临界区和软件复位 |
| Time | SysTick 单调时钟、微秒/毫秒时间和阻塞延时 |
| GPIO | 输入输出、模拟模式、读写和中断回调 |
| UART | UART0-3，轮询式 8N1 收发和超时 |
| SPI | SPI0-1 主机模式，Mode 0/3 和全双工轮询传输 |
| I2C | I2C0 主机模式，7 位地址和组合写读 |
| ADC | 外部通道 0-13、电池和温度通道 |
| Flash | 32 KiB Data-Flash 的安全边界读写和擦除 |
| PWM | PWM4-11，对外编号为 `CHIP_PWM_0` 到 `CHIP_PWM_7` |
| Power | DCDC 开关和 idle 等待 |
| USB Device | USBFS、EP0-EP7、异步分包传输、总线事件和 STALL |
| BLE Peripheral | BLE 5.x Peripheral 初始化、广播控制、TMOS 轮询和连接状态事件 |
| BLE HID Keyboard | HID-over-GATT Report/Boot Protocol、输入通知和 LED 输出报告 |

## 已知边界

- `chip_time_init()` 独占 SysTick 计时状态；FreeRTOS 固件不可调用它，由 RTOS SysTick hook
  推进调度 tick，并使用 `xTaskGetTickCount()` 作为时间源。
- CH585 只支持 SPI Mode 0/3；Mode 1/2 返回 `CHIP_ERROR_UNSUPPORTED`。
- 所有 PWM 通道共享频率，已有通道工作时不能配置不同频率。
- GPIO PB8/PB9 与 PB22/PB23 共享中断映射，冲突配置返回 `CHIP_ERROR_BUSY`。
- USB Device port 使用全速 `USB` 控制器并独占 `USB_IRQHandler`，尚不覆盖 `USB2` 高速控制器。
- USBFS 等时、批量和中断端点最大包长为 64 字节；当前不支持 Isochronous 端点。
- USB 协议栈必须先注册事件回调并准备 EP0，再显式调用 `chip_usb_device_connect()`。
- USB 事件回调运行在中断上下文，描述符解析和类请求处理必须留在上层协议栈。
- BLE 当前使用原厂 `BLE_LIB/libCH58xBLE_PERI.a`，仅封装 Peripheral 角色；Central、Observer、
  自定义 GATT Profile、配对和 SNV 持久化尚未提供通用接口。
- BLE 默认占用约 6 KiB 协议栈堆、BLEL 中断、RTC 中断和内部 32 kHz RC 时钟。应用必须频繁
  调用 `chip_ble_process()`，不可在主循环中执行长时间阻塞操作。
- 当前 BLE 实现使用芯片出厂 MAC，支持 -20、-15、-10、-8、-5、-3、-1、0、1、2、3、4 dBm。
- BLE/RF 必须使用外部 32 MHz 高速晶振作为系统时钟源；使用内部 HSI 时初始化返回
  `CHIP_ERROR_UNSUPPORTED`。晶振负载电容由板级 `external_crystal_load_pf` 配置。
- BLE HID 当前使用 Just Works 会话加密，不保存 bonding 信息；设备复位后主机需要重新连接
  或配对。持久化配对需要后续为 BLE_LIB 接入独立 SNV Data-Flash 分区。

## 初始化顺序

```c
chip_system_config_t system_config = {
    .source = CHIP_CLOCK_INTERNAL,
    .core_clock_hz = 62400000U,
    .external_crystal_load_pf = 0U,
};

chip_system_init(&system_config);
chip_time_init();
```

必须先设置系统时钟，再初始化时间模块。时间模块初始化后再次修改系统时钟会返回
`CHIP_ERROR_BUSY`。UART、SPI、I2C 和 ADC 使用有限超时时，时间模块也必须已经初始化。
所有接口的超时单位均为微秒；`CHIP_TIMEOUT_NONE` 表示不等待，`CHIP_TIMEOUT_FOREVER`
表示永久等待。

外设引脚选择和 `GPIOPinRemap` 属于板级配置。port 不会擅自决定 UART、SPI、I2C 或 PWM
使用哪组复用引脚。

USBFS Device 初始化后保持断开。上层协议栈应处理 `CHIP_USB_EVENT_SETUP_RECEIVED` 和
`CHIP_USB_EVENT_BUS_RESET`，准备好 EP0 后调用 `chip_usb_device_connect()`。IN/OUT 缓冲区
在收到 `CHIP_USB_EVENT_TRANSFER_COMPLETE` 前必须保持有效。

## 构建

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake
cmake --build build
```

USB 寄存器状态机的主机测试：

```sh
cmake -S tests/host -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

应用 target 建立后，使用下列函数加入 CH585 启动文件、链接脚本和芯片库：

```cmake
add_executable(example_app main.c)
configure_ch585_executable(example_app)
```

BLE 应用改用专用装配函数，它会额外链接 Peripheral 协议栈和 WCH Link Layer 调度汇编：

```cmake
add_executable(example_ble main.c)
configure_ch585_ble_executable(example_ble)
```

基础 Peripheral 初始化示例：

```c
chip_ble_peripheral_config_t config;

chip_ble_peripheral_config_default(&config);
config.device_name = "CH585 Demo";
chip_ble_peripheral_init(&config);

for (;;) {
    chip_ble_process();
}
```
