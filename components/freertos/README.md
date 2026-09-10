# FreeRTOS

本组件接入上游 FreeRTOS-Kernel V11.3.1，并在 `portable/wch/ch585/` 保存项目维护的 QingKe
V4C port。`FreeRTOS-Kernel/` 是干净的上游嵌套仓库，平台适配不得写入其中。

当前 CH585 port 基于 WCH EVT 的 V10.5.1 移植模型适配到 V11.3.1：

- 使用 CH585 SysTick 产生 1 kHz tick。
- 使用 PFIC 软件中断执行任务上下文切换。
- 使用 QingKe 硬件压栈和 WCH fast interrupt ABI。
- 通过 FreeRTOS 专用强 `handle_reset` 在机器模式启动，普通固件仍使用 SDK 默认启动入口。
- 支持单核抢占、同优先级时间片、`taskYIELD()` 和 `vTaskDelay()`。
- 默认使用 `heap_4`，FreeRTOS heap 为 8 KiB。

应用链接 `platform::freertos`：

```cmake
target_link_libraries(app PRIVATE platform::freertos)
```

默认 CPU 时钟为 62.4 MHz、tick 为 1 kHz，可按 target 覆盖：

```cmake
target_compile_definitions(freertos_config INTERFACE
    PLATFORM_FREERTOS_CPU_CLOCK_HZ=52000000U
    PLATFORM_FREERTOS_TICK_RATE_HZ=1000U
    PLATFORM_FREERTOS_HEAP_SIZE=12288U)
```

这些宏必须同时应用到内核和应用，不能只修改应用源文件看到的值，因此通过
`freertos_config` 的 interface 定义进行项目级配置。

```c
xTaskCreate(worker, "worker", 160, NULL, 1, NULL);
vTaskStartScheduler();
```

## 资源所有权

FreeRTOS 固件由 RTOS 独占 SysTick 和 SWI，不能调用 `chip_time_init()`。需要系统时间时使用
`xTaskGetTickCount()`；需要延时时使用 `vTaskDelay()` 或 `vTaskDelayUntil()`。

当前版本未启用 tickless idle。普通外设 ISR 仍使用现有 CH585 fast interrupt 入口，ISR 中
只能调用以 `FromISR` 结尾的 FreeRTOS API，并在需要时执行 `portYIELD_FROM_ISR()`。

FreeRTOS 与 WCH BLE_LIB 组合时必须使用 `add_ch585_freertos_ble_support()`，不能使用普通
`configure_ch585_ble_executable()`。组合适配在 BLE LLE 内部上下文运行期间屏蔽 SysTick 和
SWI，返回后恢复，以防 FreeRTOS 在 BLE 私有栈上切换任务。TMOS 应放在仅高于 Idle 的常驻
任务中持续运行。该组合目前属于实验性支持，已经完成编译、链接和中断入口检查，仍需目标
板长时间压力验证。

`platform_log` 当前使用单个静态格式化缓冲区，不可由多个任务并发调用。多任务应用应在
日志 backend 外增加 mutex，或集中到单独日志任务；`freertos_tasks` demo 只有 report 任务
写日志。
