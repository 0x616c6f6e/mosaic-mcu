# FatFs

该目录将上游 FatFs R0.16 接入平台构建，提供 `platform::fatfs` 和 `fatfs::fatfs` CMake
target。调用方应包含 `<platform_fatfs.h>`，以确保自身与 `ff.c` 使用同一套配置。

当前配置支持一个读写卷、512 字节固定扇区、短文件名和 `f_mkfs()`，关闭 LFN、exFAT、RTC
时间戳和线程安全支持。应用必须提供 `disk_initialize()`、`disk_status()`、`disk_read()`、
`disk_write()` 和 `disk_ioctl()`。

FatFs 假定底层块设备可覆盖写。裸 SPI NOR 需要 FTL 才能获得合理的掉电安全、磨损均衡和
写放大；`apps/demo/spi_flash_fatfs/` 中的 4 KiB 读改擦写仅用于功能验证。
