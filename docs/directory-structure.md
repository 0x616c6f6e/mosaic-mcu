# 通用平台目录结构与职责

## 1. 目标结构

```text
mosaic-mcu/
├── CMakeLists.txt
├── apps/                              # 最终应用，每个应用独立装配平台能力
│   ├── demo/                          # 当前芯片的外设与组件验证程序
│   └── keyboard/                      # 键盘应用
│       ├── CMakeLists.txt
│       ├── include/
│       ├── src/
│       └── components/                # 矩阵、消抖、键位、HID 等专属模块
├── boards/                            # PCB/开发板资源描述
│   └── <board_name>/
│       ├── CMakeLists.txt
│       ├── include/board.h
│       └── src/board.c
├── chips/                             # SoC SDK 与二次封装
│   └── wch/ch585/
│       ├── CMakeLists.txt
│       ├── SDK.md
│       ├── port/
│       │   ├── include/                # CH585 提供给上层的通用 API
│       │   │   ├── chip_status.h
│       │   │   ├── chip_system.h
│       │   │   ├── chip_time.h
│       │   │   ├── chip_gpio.h
│       │   │   ├── chip_uart.h
│       │   │   ├── chip_spi.h
│       │   │   ├── chip_i2c.h
│       │   │   ├── chip_adc.h
│       │   │   ├── chip_flash.h
│       │   │   ├── chip_pwm.h
│       │   │   ├── chip_power.h
│       │   │   ├── chip_usb_device.h
│       │   │   ├── chip_ble.h
│       │   │   └── chip_ble_hid_keyboard.h
│       │   └── src/                    # CH585 API 实现和私有头文件
│       └── sdk/                        # 原厂快照，原则上不修改
├── components/                        # 跨应用复用且与具体业务无关的组件
│   ├── log/                            # 分级日志、格式化与可注入输出 backend
│   ├── json/                           # jsmn 零分配 JSON tokenizer
│   ├── freertos/                       # FreeRTOS 上游内核和芯片 portable layer
│   └── tinyusb/
│       ├── CMakeLists.txt              # 平台构建入口
│       ├── dcd_chip.c                  # Chip USB 到 TinyUSB DCD 的适配
│       └── tinyusb/                    # TinyUSB 上游源码
├── cmake/                             # 平台公共 CMake 模块
├── tests/
│   ├── unit/                          # 主机单元测试
│   ├── contract/                      # 不同芯片 port 的 API 契约测试
│   └── fakes/                         # 主机侧替身
├── tools/
│   └── wch/
│       ├── Toolchain/                  # WCH GCC8/GCC12/GCC15
│       └── OpenOCD/                    # WCH OpenOCD
└── docs/
```

目标结构不要求提前创建空模块。出现第一个源文件或构建目标时再建立目录，不使用大量
`.gitkeep` 维持空骨架。

## 2. 分层边界

```text
apps/<application>
├── board
├── application components -> <chip_*.h>
└── selected chip port -> vendor SDK
```

依赖规则：

1. 应用负责装配，不把业务代码放入芯片层。
2. 应用和通用组件通过 `<chip_*.h>` 使用当前构建选择的 port，不包含厂商头文件。
3. 板级代码保存引脚、晶振、外设映射和电气极性，不实现寄存器操作。
4. `port/src/` 是唯一允许同时包含 `<chip_*.h>` 和厂商 SDK 头文件的位置。
5. `sdk/` 不依赖 port、组件或应用。
6. 根目录 `components/` 只能保存跨应用能力；键盘专属组件放在 `apps/keyboard/`。

## 3. Chip API 放置规则

每颗芯片的公共头文件位于自己的 `port/include/`。构建系统只把当前选中 port 的
include 路径传递给上层 target，因此调用方始终使用以下形式：

```c
#include <chip_gpio.h>
#include <chip_time.h>
```

新增芯片时，其 `chip_*.h` 接口必须与已有接口保持源码兼容。由于接口不再集中保存，必须
通过 `tests/contract/` 编译测试约束头文件、函数签名、状态码和行为语义，不能在某颗芯片下
自行增加同名函数的私有含义。

port 公共头文件不得出现厂商句柄、寄存器结构、中断号或私有错误码。CH585 私有辅助头文件
放在 `port/src/`，不加入 `chip_api` 的公开 include 路径。

## 4. 厂商 SDK 与 Port

```text
chips/<vendor>/<soc>/
├── SDK.md
├── sdk/       原厂快照、启动文件、链接脚本、预编译库
└── port/      项目维护的参数校验、类型转换、超时和中断桥接
```

- SDK 源码原则上不修改，版本、来源、许可证和补丁记录在 `SDK.md`。
- 构建时显式列出使用的 SDK 源文件，不使用 `file(GLOB ...)`。
- SDK include 只提供给对应 port，不能成为全局 include 路径。
- 项目修改过的链接脚本应离开 SDK 快照并记录来源。

## 5. 组件与应用

根目录组件负责跨芯片策略，例如资源占用、事件队列、存储策略和协议栈。Chip API 只负责
稳定表达硬件能力，组件通过组合这些接口提供更高层能力。

`apps/demo/` 仅用于验证当前选中芯片及组件，不提供产品功能。键盘矩阵、消抖、键位映射、
HID 报告和组合键均属于 `apps/keyboard/`。如果某个模块后来被
多个应用真实复用，再将其移动到根目录 `components/`，不要提前把业务模块标记为通用。

## 6. 板级目录

板型是“芯片 + PCB 连接 + 产品选项”的集合，适合保存：

- 芯片型号、封装和晶振频率。
- 外设复用选择、引脚和电平极性。
- Flash 分区及 bootloader 布局。
- 默认启用的通信接口和日志后端。

板型不包含通用外设驱动，也不复制芯片初始化代码。

## 7. CMake Target

| 目录 | Target |
| --- | --- |
| `chips/wch/ch585/port/include/` | `chip_api` / `chip::api` |
| `chips/wch/ch585/sdk/` | `wch_ch585_sdk` |
| `chips/wch/ch585/port/src/` | `ch585_port` / `chip::selected` |
| `components/log/` | `platform_log` / `platform::log` |
| `components/json/` | `json_jsmn` / `platform::json` / `jsmn::jsmn` |
| `components/freertos/` | `freertos_kernel` / `platform::freertos` |
| `boards/<board>/` | `board_<board>` |
| `apps/<application>/` | 最终 ELF target |

最终应用通过 `configure_ch585_executable(<target>)` 获得 CH585 port、启动文件和链接脚本。

## 8. 文件归属判断

| 问题 | 放置位置 |
| --- | --- |
| 只是厂商交付内容吗？ | `chips/<vendor>/<soc>/sdk/` |
| 换一颗 SoC 后实现一定不同吗？ | `chips/<vendor>/<soc>/port/` |
| 换 PCB 后会改变吗？ | `boards/<board>/` |
| 是跨芯片资源策略吗？ | `components/<feature>/` |
| 能被多个应用复用且不含业务语义吗？ | `components/` |
| 只服务于某个产品或场景吗？ | `apps/<application>/` |

无法明确归属时，不使用 `common/`、`misc/` 或 `utils/` 暂存，应先明确模块的调用者和依赖。
