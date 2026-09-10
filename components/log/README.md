# Platform Log

`platform_log` 是与芯片无关的同步日志组件。它负责日志级别、tag、时间戳、格式化和换行，
实际传输由应用注入的 backend 完成，因此可以接到 UART、USB CDC、RTT 或测试缓冲区。

```c
platform_log_config_t config = {
    .backend = board_log_write,
    .backend_context = NULL,
    .timestamp = board_millis,
    .timestamp_context = NULL,
    .level = PLATFORM_LOG_LEVEL_INFO,
};

platform_log_init(&config);
LOG_INFO("app", "started, id=0x%02X", chip_id);
```

默认单条记录缓冲区为 256 字节，可在编译 `platform_log` target 时通过
`PLATFORM_LOG_BUFFER_SIZE` 修改。记录超长时仍发送截断后的完整 CRLF 行，并返回
`PLATFORM_LOG_ERROR_TRUNCATED`。为避免嵌入式固件链接完整 stdio，格式化器支持
`%s`、`%c`、`%d/%i`、`%u`、`%x/%X`、`%p`、整数长度修饰符、字段宽度和零填充，不支持
浮点、精度和左对齐。

组件使用一个静态格式化缓冲区，当前为同步、不可重入实现，不应从中断或多个并发执行
上下文直接调用。

通过 target 编译定义可以移除低级别日志及其参数求值：

```cmake
target_compile_definitions(app PRIVATE
    PLATFORM_LOG_COMPILE_LEVEL=PLATFORM_LOG_LEVEL_INFO_VALUE)
```
