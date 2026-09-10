# wchisp

本目录包含项目使用的 `wchisp` 主机端烧录程序。它用于 WCH USB/UART ISP，不用于
WCH-Link 调试器。

| 项目 | 值 |
| --- | --- |
| 上游 | <https://github.com/ch32-rs/wchisp> |
| 版本 | 0.3.0 |
| Commit | `cefd8707df345f1fbd7795e15367281f440bbf05` |
| 许可证 | GPL-2.0 |
| 主机平台 | Linux x86_64 |

当前二进制通过以下命令从上游源码构建：

```sh
cargo install --git https://github.com/ch32-rs/wchisp --locked \
  --root tools/wch/wchisp
```

升级版本时应重新执行安装命令并同步更新本文件中的版本和 commit。其他主机平台需要从
源码重新构建，不能直接使用 `bin/wchisp`。

Linux 首次使用时安装 USB ISP 权限规则：

```sh
tools/wch/wchisp/install-udev-rule.sh
```

规则仅匹配 WCH USB ISP `1a86:55e0`，授予 `plugdev` 组读写权限，不开放给所有用户。
