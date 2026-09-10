# TinyUSB Component

外层目录保存平台构建入口和 DCD adapter，将 TinyUSB Device Controller Driver API 映射到
`chip_usb_device`，不直接访问 CH585 寄存器。TinyUSB 上游源码位于内层 `tinyusb/`，当前
版本为 0.21.0，commit 为 `dae3f9a366bfcddbf9dcf1b48d7500286a849539`。

`add_tinyusb_hid_device(target, config_directory)` 为指定应用加入 TinyUSB device core、USBD、
FIFO、HID class 和 DCD adapter。每个应用必须在 `config_directory` 提供自己的
`tusb_config.h`。

FreeRTOS 应用使用 `add_tinyusb_freertos_hid_device(target, config_directory)`，并在配置头中
设置 `CFG_TUSB_OS=OPT_OS_FREERTOS`。此模式使用 FreeRTOS 静态 queue/semaphore，必须在
调度器启动后的 USB task 中调用 `tusb_init()`。

当前适配边界：

- 单个 USBFS Device root hub，Full Speed。
- 支持 Control、Bulk 和 Interrupt 端点。
- TinyUSB 中断事件由 Chip USB ISR 投递到 `tud_task()`。
- 不支持 Isochronous 端点和 SOF 回调。
- 外层适配代码与内层 TinyUSB 上游工作树相互隔离。

版本管理注意：内层 `tinyusb/` 是独立 Git 仓库。根项目正式提交前应将其登记为 Git
submodule，确保其他开发者克隆项目后可以取得同一个 commit；不要只提交缺少 `.gitmodules`
的嵌入仓库记录。
