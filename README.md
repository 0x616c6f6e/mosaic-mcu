# mosaic-mcu

`mosaic-mcu` 是面向资源受限 MCU 的通用嵌入式平台。平台负责隔离 SoC 厂商 SDK，向应用和可复用组件
提供一致的 Chip API；键盘只是 `apps/keyboard/` 下的一个应用场景，不参与平台命名和底层
接口设计。

当前已完成 WCH CH585 第一版基础外设适配：

- 原厂 SDK：`chips/wch/ch585/sdk/`
- 通用接口：`chips/wch/ch585/port/include/`
- CH585 实现：`chips/wch/ch585/port/src/`
- WCH 工具：`tools/wch/`

## 文档

- [平台架构与移植规范](docs/early-stage-architecture.md)
- [目录结构与职责](docs/directory-structure.md)
- [CH585 Port 使用说明](chips/wch/ch585/port/README.md)
- [CH585 Demos](apps/demo/README.md)
- [使用 wchisp 烧录固件](docs/flashing-with-wchisp.md)

## 当前状态

- [x] 隔离 CH585 原厂 SDK 与自研 port
- [x] 完成 System、Time、GPIO、UART、SPI、I2C、ADC、Flash、PWM、Power、USBFS Device 和 BLE Peripheral 封装
- [x] 接入 TinyUSB 0.21 Device/HID 组件和 Chip USB DCD adapter
- [x] 增加与芯片无关的分级 LOG API 和 CH585 UART backend
- [x] 接入 jsmn 1.1.0 零分配 JSON tokenizer
- [x] 接入 FreeRTOS-Kernel 11.3.1 和 CH585 QingKe 任务调度 port
- [x] 增加标准 JEDEC SPI NOR Flash 读、写、擦除组件
- [x] 接入 littlefs 2.11.3，并增加 SPI NOR 持久化示例
- [x] 接入 FatFs R0.16，并增加 SPI NOR 块设备示例
- [x] 增加 FreeRTOS + TinyUSB HID + BLE Peripheral 组合验证固件
- [x] 为组合固件增加标准 BLE HID Keyboard Report/Boot Protocol 服务
- [x] 使用仓库内 WCH GCC12 完成交叉编译和完整链接
- [ ] 在 CH585 开发板运行外设契约测试
- [ ] 建立跨芯片 API 契约测试
- [ ] 增加第一个板级目标
- [ ] 在 `apps/keyboard/` 实现键盘应用

平台层不得包含键盘业务，也不得通过公共 API 传播 `CH58x_*.h`、寄存器宏或厂商类型。
