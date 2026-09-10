# CH585 SDK 记录

该目录保存 WCH CH585 原厂标准外设库快照。项目适配代码不得放入 `sdk/`，应放入
`port/`。

| 项目 | 当前值 |
| --- | --- |
| SDK 名称 | WCH CH585 StdPeriphDriver |
| SDK 版本 | 混合版本，待确认原始发行包版本 |
| 获取地址 | 待确认 |
| 导入日期 | 待确认 |
| 许可证 | 待确认 |
| 本地补丁 | 暂无记录 |

## BLE_LIB

| 项目 | 当前值 |
| --- | --- |
| 头文件版本 | `CH585_BLE_LIB_V1.4` / v1.40，2025-02-07 |
| Peripheral 库 | `BLE_LIB/libCH58xBLE_PERI.a` |
| 完整角色库 | `BLE_LIB/libCH58xBLE.a`（当前未链接） |
| ABI | ELF32 RISC-V、RVC、soft-float ABI |
| 调度入口 | `BLE_LIB/ble_task_scheduler.S` |

BLE 库为厂商预编译二进制，只能用于 WCH 对应 MCU；版本、来源和再分发许可仍需根据原始
EVT/SDK 发布包补全。当前 port 选择 Peripheral-only 库，避免将 Central/Observer 能力和更大
资源占用默认带入应用。`CH585BLE_ROMx.hex` 等 ROM 方式文件未参与当前链接。

正式发布或向第三方分发固件源码前，必须补全版本、来源和许可证信息。如需修改原厂文件，
应记录文件、原因和补丁，不直接混入芯片适配实现。

当前快照中的文件头版本并不完全一致，例如标准外设文件主要标记为 2021 年版本，I2C、
SFR 和 ISP 库文件包含后续更新。因此不能只用单个源文件的版本号代替 SDK 发行包版本。
