# Plan-11b — ini2arr 接入

## 核心问题

当前 Config_Init / Config_Get / Config_Set / Config_Shutdown 均为空实现，无法读写 state.ini。

## 分析现状

- 前置依赖：`src/utils/ini2arr.c` / `ini2arr.h` 已完成（4 测试全通过）
- 前置依赖：`src/config/config.h` / `config.c` 骨架已就绪（5 API 声明 + 空实现）
- 现状：config 模块的 5 个 API 都是 return 0 或空操作，实际调用毫无作用
- 产出：11b 完成后，Config_Init 能从 state.ini 加载数据，Config_Get 能查到值，Config_Set 能行级写入并触发回调

## 方案选型

- **方案 A（选中）：直接实现**
  - 用 ini2arr 填充 Config 内部数据结构，5 个 API 逐个落地
  - 线性扫描 entries 匹配 key，按 ini 条目数（≤50）性能足够
  - 符合分层规则：config 层仅依赖 ini2arr（utils），不依赖 app/platform/editor/ui/render/storage

- **方案 B（排除）：先改为 hashtable 再实现**
  - 将 Config 内部数据结构从数组改为哈希表，再用 ini2arr 填充
  - 排除理由：当前 ini 条目数不超过 50，线性扫描性能完全够；引入 hashtable 增加实现复杂度和代码体积，无实际收益

## 拆解执行

### 主链路

```
App_Init → Config_Init → ini2arr("state.ini") → entries 内存表
Config_Get(key) → 循环 entries → 匹配 → 返回 val
Config_Set(key, val) → 更新内存 entries → ini2arr_write → on_change 回调
```

## 设定边界

### 范围

- 实现 Config_Init / Config_Get / Config_Set / Config_Shutdown 四个非空函数
- `CMakeLists.txt` 加入 config.c + ini2arr.c 源文件
- 创建 `test/test_config.c` 单元测试
- Config_OnChange 注册 API 保持骨架空实现（11d 使用）

### 不做

- 不改 app.c（11c 做）
- 不注册 on_change 回调（11d 做）
- 不改 state.ini 的键名或文件格式
- 不改 ini2arr 模块

### 不修改的文件

src/app/*、src/ui/*、src/editor/*、src/core/*、src/platform/*、src/render/*、src/storage/*

## 落地方案

### Config 内部数据结构

```c
#include "config.h"
#include "../utils/ini2arr.h"
#include <string.h>

static struct {
    Ini2Arr arr;
    char    path[512];
    ConfigCallback cb;
} g_config;

#define ARR (&g_config.arr)
```

### Config_Init

```c
int Config_Init(const char* path)
{
    /* 避免重复调用泄漏 */
    if (ARR->entries)
        ini2arr_free(ARR);
    strncpy(g_config.path, path, sizeof(g_config.path) - 1);
    return ini2arr(path, ARR);
}
```

### Config_Get

```c
int Config_Get(const char* key, int default_val)
{
    for (int i = 0; i < ARR->count; i++)
        if (strcmp(ARR->entries[i].key, key) == 0)
            return ARR->entries[i].val;
    return default_val;
}
```

### Config_Set

```c
int Config_Set(const char* key, int new_val)
{
    if (!ARR->entries)
        return 1;

    int old_val = Config_Get(key, -1);
    if (old_val == new_val)
        return 0;

    /* update memory — find existing or append */
    for (int i = 0; i < ARR->count; i++)
        if (strcmp(ARR->entries[i].key, key) == 0)
        { ARR->entries[i].val = new_val; goto write; }

    /* key not found → append new entry to memory */
    strncpy(ARR->entries[ARR->count].key, key, INI2ARR_KEY_LEN);
    ARR->entries[ARR->count].val = new_val;
    ARR->count++;

write:
    /* row-level write (handles both existing and new keys) */
    if (ini2arr_write(g_config.path, key, new_val) != 0)
        return 1;

    /* callback */
    if (g_config.cb)
        g_config.cb(key, old_val, new_val);

    return 0;
}
```

### Config_Shutdown

```c
void Config_Shutdown(void)
{
    if (ARR->entries)
        ini2arr_free(ARR);
}
```

### CMakeLists.txt 修改

在 `src/storage/storage_write.c` 之后、`src/ui/editor_view.c` 之前插入：

```cmake
    src/config/config.c
    src/utils/ini2arr.c
```

## 验收标准

- [ ] `gcc -o test_config test/test_config.c src/config/config.c src/utils/ini2arr.c` 编译通过
- [ ] 测试 7 条全部 PASS
- [ ] 分层核实：config.c 中不包含 app/platform/editor/ui/render/storage 的头文件引用

## 推进步骤

### 方案 A：直接实现 [selected]

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | `CMakeLists.txt` SOURCES 中插入 `src/config/config.c` 和 `src/utils/ini2arr.c` | 文件被编译系统识别 | `cmake . 2>&1 \| grep config` |
| 2 | 替换 `src/config/config.c` 为完整实现（Config_Init / Get / Set / Shutdown） | 4 个 API 非空 | `gcc -fsyntax-only src/config/config.c` |
| 3 | 创建 `test/test_config.c` | 测试文件存在，包含 7 条断言 | 见测试表 |
| 4 | 编译并运行测试 | 7 条全部 pass | `gcc -o test_config test/test_config.c src/config/config.c src/utils/ini2arr.c && ./test_config` |

### 方案 B：hashtable（备选）

（本方案未选中，推进步骤为空）
