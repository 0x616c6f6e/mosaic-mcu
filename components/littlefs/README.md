# littlefs

该目录将上游 littlefs 2.11.3 接入平台构建，提供 `platform::littlefs` 和
`littlefs::littlefs` CMake target。

平台配置定义了 `LFS_NO_MALLOC`，调用方必须为文件系统读缓存、写缓存、lookahead 缓存以及
每个打开文件的缓存提供静态存储。上游 debug/warn/error 的 stdio 输出也已关闭，应用应记录
并处理 littlefs 返回的负错误码。

实际块设备由应用通过 `struct lfs_config` 的 `read`、`prog`、`erase` 和 `sync` 回调注入。
SPI NOR 示例见 `apps/demo/spi_flash_littlefs/`。
