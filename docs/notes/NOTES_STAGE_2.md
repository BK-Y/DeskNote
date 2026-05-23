# DeskNote 学习笔记 - 阶段 2：编辑器与文件 I/O

> Rich Edit 文本编辑

---

## 17. 阶段 2：Rich Edit 文本编辑

### 17.1 Rich Edit 控件简介

Rich Edit 是 Windows 内置的富文本编辑控件，位于 `riched20.dll`。它自带：

| 功能 | 是否需要自己写 |
|------|--------------|
| 打字、删除、光标移动 | ❌ 自带 |
| 文本选中 | ❌ 自带 |
| 复制/粘贴 | ❌ 自带 |
| 撤销/重做（Ctrl+Z/Y） | ❌ 自带 |
| 滚动条 | ❌ 自动出现 |
| Unicode/中文输入 | ❌ 自带 |
| 富文本格式（加粗、颜色） | ❌ 自带，CHARFORMAT2 控制 |
| Markdown 解析 | ✅ 需要自己用 md4c |
| 复选框点击切换 | ✅ 需要自己实现 |

### 17.2 嵌入步骤

```
① LoadLibraryW(L"riched20.dll")  加载系统 DLL
② CreateWindowExW(L"RichEdit20W")  创建控件
③ 调整 WM_SIZE 中控件大小
④ 用 SetWindowTextW / GetWindowTextW 读写内容
```

### 17.3 关键 API

| API | 作用 |
|-----|------|
| `LoadLibraryW(L"riched20.dll")` | 加载 Rich Edit 库 |
| `CreateWindowExW(0, L"RichEdit20W", ...)` | 创建 Rich Edit 控件 |
| `SetWindowTextW(hEdit, text)` | 设置文本内容 |
| `GetWindowTextW(hEdit, buf, len)` | 获取文本内容 |
| `SendMessage(hEdit, EM_SETCHARFORMAT, ...)` | 设置字体、颜色、加粗等格式 |
| `SendMessage(hEdit, EM_GETLINECOUNT, ...)` | 获取总行数 |
| `SendMessage(hEdit, EM_LINEINDEX, i, 0)` | 获取第 i 行的字符索引 |

### 17.4 编辑模式 vs 预览模式

阶段 2 实现两种模式的切换：

| 模式 | 用户看到 | 底层 |
|------|---------|------|
| 编辑模式 | 原始 Markdown 源码 | Rich Edit 纯文本，无格式 |
| 预览模式 | 渲染后的效果（加粗、标题、复选框） | Rich Edit 富文本，有格式 |

切换快捷键：双击标题栏（复用现有逻辑）或 Ctrl+P。

---

## 18. md4c 集成与验证

### 18.1 md4c 编译方式

md4c 以静态库形式编译，通过 CMake 集成：

```cmake
# 静态库目标
add_library(md4c STATIC lib/md4c/md4c-master/src/md4c.c)
target_include_directories(md4c PUBLIC lib/md4c/md4c-master/src)

# 关键：必须定义 MD4C_USE_UTF16
target_compile_definitions(md4c PRIVATE MD4C_USE_UTF16)

# 应用链接
target_link_libraries(desknote md4c ...)
```

### 18.2 MD4C_USE_UTF16 的作用

md4c 默认使用 UTF-8（`MD_CHAR = char`，1 字节）。Windows 上 Rich Edit 使用 UTF-16（`wchar_t`，2 字节）。必须定义 `MD4C_USE_UTF16` 让 md4c 内部也用 UTF-16，否则：

| 使用方 | MD_CHAR 大小 | 后果 |
|--------|-------------|------|
| md4c 库（未定义 UTF16） | 1 字节 | 把 wchar_t* 当 char* 处理，解析结果全错 |
| md_parser.c（定义了 UTF16） | 2 字节 | 传给 md_parse 的字符串被误解 |
| 两者不一致 | — | text 指针偏移量算错，span 永远追踪不到 |

修复方式：在 CMake 中给 md4c 库加上 `target_compile_definitions(md4c PRIVATE MD4C_USE_UTF16)`，确保库本身和调用方使用相同的字符宽度。

### 18.3 解析器架构

```
md_parse(text, len, &parser, &ctx)
    ↓
逐块扫描文本，触发回调：
    ├── enter_block / leave_block  → 段落结构（当前未使用）
    ├── enter_span / leave_span    → 追踪粗体/斜体/代码/删除线
    └── text_cb                    → 获取文本内容 + 扫描自定义标签
    ↓
回调收集的结果 → MDFormat[] 数组
```

### 18.4 span 追踪方式

```c
enter_span:
    ctx->span_type = type     // 记录当前 span 类型
    ctx->span_beg = -1        // 等待 text_cb 设置起始位置

text_cb:
    off = text - ctx->base    // 计算绝对偏移
    if (ctx->span_type > 0) {
        扩展 span_beg/span_end 边界
    }
    同时扫描 @ ! # 标签

leave_span:
    if (span_beg >= 0)
        add_fmt(mapped_type, span_beg, span_end)
    ctx->span_type = 0
```

### 18.5 text_cb 内同时扫描自定义标签

不需要二次遍历全文。md4c 把文本逐块喂给 text_cb，在每一块内直接扫描：

```c
text_cb("买鸡蛋 @today !high")
    ↓
扫描当前字符串中的 @ ! #：
  @ 从位置 4 → @today → MD_FMT_DATE
  ! 从位置 11 → !high → MD_FMT_PRIORITY
```

### 18.6 验证结果

测试文本：`**bold** *italic* `code` @today !high #tag``

| 格式 | 位置 | 文本 | 来源 |
|------|------|------|------|
| BOLD | [2-6] | bold | `**bold**` |
| CODE | [19-23] | code | `` `code` `` |
| DATE | [25-31] | @today | 自定义标签扫描 |
| PRIORITY | [32-37] | !high | 自定义标签扫描 |
| TAG | [38-42] | #tag | 自定义标签扫描 |

### 18.7 已知问题

- `*italic*` 在测试中未被 md4c 识别为 MD_SPAN_EM（待排查）
- `enter_span`/`leave_span` 回调必须配对使用，否则 span 追踪会丢失边界
- `MD4C_USE_UTF16` 必须在编译 md4c 库本身时定义，仅在调用方定义不够

---

*跟随开发阶段同步更新。*
