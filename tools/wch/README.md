# WCH Tools

本目录保存 WCH 工具的版本清单、安装说明和项目维护的辅助脚本。编译器、OpenOCD 和主机端
可执行文件是本机安装产物，不进入 Git 历史。

## RISC-V GCC12

默认工具链安装路径：

```text
tools/wch/Toolchain/RISC-V Embedded GCC12/
```

也可以在首次配置时指定外部安装：

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake \
  -DWCH_TOOLCHAIN_ROOT=/opt/wch-gcc12
```

WCH 工具包版本信息见 `Toolchain/sub_manifest.json` 和 `OpenOCD/sub_manifest.json`。

## wchisp

安装到默认位置：

```sh
cargo install --git https://github.com/ch32-rs/wchisp --locked \
  --root tools/wch/wchisp
```

也可以通过 `-DWCHISP_EXECUTABLE=/path/to/wchisp` 使用系统安装。USB udev 规则和安装脚本位于
`wchisp/`。
