# Markdown 实时渲染 - 技术方案

## 一、整体架构

```
用户按键
    ↓
WM_CHAR / WM_KEYDOWN
    ↓
检测是否特殊字符（# * ` @ ! - [ ] 回车 退格 删除）
    ↓
否 → 不触发，编辑器照常打字
是 → 启动 250ms 防抖定时器
         ↓
    定时器到期 → 取编辑器全文
         ↓
    md_parse() 逐块解析，触发回调
         ↓
    收集 MDFormat[] → 清除旧格式 → 逐段应用新格式
```

## 二、触发渲染的条件

### 触发渲染的特殊键

| 按键 | 原因 |
|------|------|
| `#` `*` `` ` `` | Markdown 语法标记 |
| `@` `!` | 自定义标签前缀 |
| `-` | 列表、复选框 |
| `[` `]` | 复选框、链接 |
| 回车 | 可能改变段落结构 |
| 退格、删除 | 可能破坏现有格式 |

### 不触发渲染的键

普通字母、数字、中文、空格、方向键、Tab——打字时格式不会因为这些键而变化，不触发渲染。

### 实现

```c
#define IS_SPECIAL_KEY(ch) ( \
    (ch) == L'#' || (ch) == L'*' || (ch) == L'`' || \
    (ch) == L'@' || (ch) == L'!' || (ch) == L'-' || \
    (ch) == L'[' || (ch) == L']' || \
    (ch) == VK_RETURN || (ch) == VK_BACK || (ch) == VK_DELETE)

case WM_CHAR:
    if (IS_SPECIAL_KEY(wp))
        PostMessage(hwnd, WM_COMMAND, EN_CHANGE, (LPARAM)g_hEditor);
    break;

case WM_KEYDOWN:
    if (wp == VK_RETURN || wp == VK_BACK || wp == VK_DELETE)
        PostMessage(hwnd, WM_COMMAND, EN_CHANGE, (LPARAM)g_hEditor);
    break;
```

## 三、防抖设计

每次特殊键按下时不立刻解析，而是重置 250ms 定时器：

```
用户快速输入特殊字符：
#  C  #  C          ← C=普通字符
↓  ↓  ↓  ↓
① ② ③ ④  ← 每次触发 EN_CHANGE，重置定时器
         ↓
     ⑤ 250ms 后定时器到期 → 执行渲染
```

```c
#define RENDER_DELAY 250

case WM_COMMAND:
    if (HIWORD(wp) == EN_CHANGE) {
        SetTimer(hwnd, IDT_RENDER, RENDER_DELAY, NULL);
        return 0;
    }

case WM_TIMER:
    if (wp == IDT_RENDER) {
        KillTimer(hwnd, IDT_RENDER);
        DoRender(g_hEditor);
    }
    return 0;
```

## 四、md4c 回调处理

### enter_span / leave_span

| md4c 类型 | MDFormat | CHARFORMAT2W |
|-----------|----------|--------------|
| MD_SPAN_EM | MD_FMT_ITALIC | `CFM_ITALIC` + `CFE_ITALIC` |
| MD_SPAN_STRONG | MD_FMT_BOLD | `CFM_WEIGHT` + `wWeight = FW_BOLD` |
| MD_SPAN_CODE | MD_FMT_CODE | `CFM_FACE` + `szFaceName = L"Consolas"` |
| MD_SPAN_DEL | MD_FMT_STRIKETHROUGH | `CFM_STRIKEOUT` + `CFE_STRIKEOUT` |
| MD_BLOCK_H(n) | MD_FMT_H1~H6 | `CFM_WEIGHT` + `CFM_SIZE` |

追踪方式：

```
enter_span → 记下 span_type，span_beg = -1
text_cb    → 计算绝对偏移 (text - base)，扩展 span_beg/span_end
              同时扫描当前文本块内的 @ ! # 标签
leave_span → 如果 span_beg >= 0，添加格式指令
```

### text_cb 中扫描 @ ! #

不需要二次遍历全文。md4c 把文本逐块喂给 text_cb，在每一块内扫描：

```
text_cb("买鸡蛋 @today !high")
    ↓
扫描当前字符串：
  @ 从 4 开始 → @today → 添加 MD_FMT_DATE (位置 4, 长度 6)
  ! 从 11 开始 → !high  → 添加 MD_FMT_PRIORITY (位置 11, 长度 5)
```

匹配规则：

| 标签 | 检测 | 格式颜色 |
|------|------|---------|
| `@today` `@2025-01-20` | `@` 后跟日期/关键词 | 蓝 RGB(0,120,215) |
| `!high` `!medium` `!low` | `!` 后跟优先级 | 红/橙/灰 |
| `#tag` | `#` 后直接跟文字（无空格） | 灰 RGB(120,120,120) |

## 五、格式应用

```c
void ApplyFormats(HWND hEditor, MDFormat* fmts, int n)
{
    if (n == 0) return;

    // 1. 清除旧格式
    CHARFORMAT2W plain = { sizeof(plain) };
    plain.dwMask = CFM_WEIGHT | CFM_ITALIC | CFM_SIZE |
                   CFM_COLOR | CFM_FACE | CFM_STRIKEOUT;
    SendMessage(hEditor, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&plain);

    // 2. 按位置排序
    // （md4c 回调已经按顺序输出，通常不需要再排序）

    // 3. 逐段应用
    for (int i = 0; i < n; i++) {
        CHARFORMAT2W cf = { sizeof(cf) };
        SetFormatFromType(&cf, fmts[i].type);
        SendMessage(hEditor, EM_SETSEL, fmts[i].beg, fmts[i].end);
        SendMessage(hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    // 4. 取消选中
    SendMessage(hEditor, EM_SETSEL, 0, 0);
}
```

## 六、CHARFORMAT2W 映射表

| MDFormat 类型 | dwMask | 字段 | 值 |
|-------------|--------|------|-----|
| MD_FMT_BOLD | CFM_WEIGHT | wWeight | FW_BOLD |
| MD_FMT_ITALIC | CFM_ITALIC | dwEffects | CFE_ITALIC |
| MD_FMT_CODE | CFM_FACE | szFaceName | L"Consolas" |
| MD_FMT_STRIKETHROUGH | CFM_STRIKEOUT | dwEffects | CFE_STRIKEOUT |
| MD_FMT_H1 | CFM_WEIGHT | wWeight | FW_BOLD |
| MD_FMT_H1 | CFM_SIZE | yHeight | 360 |
| MD_FMT_H2 | CFM_WEIGHT | wWeight | FW_BOLD |
| MD_FMT_H2 | CFM_SIZE | yHeight | 300 |
| MD_FMT_DATE | CFM_COLOR | crTextColor | RGB(0,120,215) |
| MD_FMT_PRIORITY | CFM_COLOR | crTextColor | 按级别红/橙/灰 |
| MD_FMT_TAG | CFM_COLOR | crTextColor | RGB(120,120,120) |

## 七、边界情况

| 场景 | 处理 |
|------|------|
| 内容为空 | 跳过渲染 |
| 超过 10000 字符 | 截断解析 |
| SCF_ALL 触发 EN_CHANGE | `g_rendering` 防递归 |
| 多个格式叠加 | 后应用的覆盖先应用的 |

## 八、执行步骤

| 步骤 | 内容 |
|------|------|
| 1 | 建立 lib/md4c/，配置 CMakeLists.txt |
| 2 | 写 md_parser.h：MDFormat 类型 + MDParse/MDFree 声明 |
| 3 | 写 md_parser.c：md4c 回调 + 自定义标签扫描 |
| 4 | 改 main.c：WM_CHAR/WM_KEYDOWN 触发 + 防抖 + ApplyFormats |
| 5 | 编译测试 |
