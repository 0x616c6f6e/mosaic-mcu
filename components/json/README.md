# JSON / jsmn

本组件接入上游 jsmn v1.1.0。上游源码保存在 `jsmn/` 嵌套仓库，平台构建和说明保存在外层，
不得直接修改上游工作树。

jsmn 是零分配、单遍扫描的 JSON tokenizer。token 只记录原始 JSON 文本中的类型和起止位置，
不会复制字符串，也不构建 DOM；JSON 文本和调用方提供的 token 数组必须在使用 token 期间
保持有效。

## CMake

应用或组件链接任一 target：

```cmake
target_link_libraries(app PRIVATE platform::json)
# 或 jsmn::jsmn
```

外层 target 使用单独的 `src/jsmn.c` 提供实现，并向消费者定义 `JSMN_HEADER`，因此多个源文件
可以安全包含 `<jsmn.h>`。平台同时启用上游 `JSMN_STRICT` 模式，收紧 primitive 的结束规则；
jsmn 仍然只是 tokenizer，调用方如需完整语义校验，还必须检查 object key 类型以及 primitive
内容。

## 使用

```c
#include <jsmn.h>

const char json[] = "{\"enabled\":true,\"rate\":1000}";
jsmn_parser parser;
jsmntok_t tokens[8];

jsmn_init(&parser);
int count = jsmn_parse(&parser, json, sizeof(json) - 1U,
                       tokens, sizeof(tokens) / sizeof(tokens[0]));
if (count < 0) {
    /* JSMN_ERROR_NOMEM / JSMN_ERROR_INVAL / JSMN_ERROR_PART */
}
```

jsmn 不解析数字、不处理字符串反转义，也不添加 `\0`。调用方需要使用 token 的
`start/end` 边界完成比较、复制和类型转换。若 JSON 来自流，可以保留同一个 parser，在收到
更多数据后继续调用 `jsmn_parse()`。

内层 `jsmn/` 是独立 Git 仓库。根项目正式提交前应登记为 submodule，确保其他开发者能够
取得 v1.1.0 对应的 commit `fdcef3ebf886fa210d14956d3c068a653e76a24e`。
