# Plan-11b — ini2arr 接入

## 目标
Config_Init 用 ini2arr 读 state.ini → 内存表。Config_Set 调 ini2arr_write 行级写回。

## 步骤

### 1. CMakeLists.txt 加源文件

```cmake
src/config/config.c
src/utils/ini2arr.c
```

### 2. Config 内部数据结构

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

### 3. 实现 Config_Init

```c
int Config_Init(const char* path)
{
    strncpy(g_config.path, path, sizeof(g_config.path) - 1);
    return ini2arr(path, ARR);
}
```

### 4. 实现 Config_Get

```c
int Config_Get(const char* key, int default_val)
{
    for (int i = 0; i < ARR->count; i++)
        if (strcmp(ARR->entries[i].key, key) == 0)
            return ARR->entries[i].val;
    return default_val;
}
```

### 5. 实现 Config_Set

```c
int Config_Set(const char* key, int new_val)
{
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

### 6. 实现 Config_Shutdown

```c
int Config_Shutdown(void)
{
    ini2arr_free(ARR);
    return 0;
}
```

### 7. 测试

建 `test/test_config.c`，包含以下测试路径：

| # | 操作 | 预期 |
|---|------|------|
| 1 | 创建 temp.ini：`mode=1` → `Config_Init` | 返回 0 |
| 2 | `Config_Get("mode", -1)` | 返回 1 |
| 3 | `Config_Get("nonexist", -1)` | 返回 -1（default_val） |
| 4 | `Config_Set("mode", 2)` | 返回 0 |
| 5 | `Config_Get("mode", -1)` | 返回 2 |
| 6 | 打开 temp.ini 检查 | `; / #` 注释行仍在，`mode=2` |
| 7 | `Config_Shutdown` | 不报错 |

```sh
gcc -o test_config test/test_config.c src/config/config.c src/utils/ini2arr.c
./test_config && echo "ALL PASS"
```

## 验证

全部测试通过。Config 层四 API + Shutdown 功能可用。

## 不做

- 不触到 app.c（11c 做）
- 不注册 on_change（11d 做）

## 分层核实

- Config 层仅依赖 ini2arr（utils），不依赖 app/platform/editor/ui/render/storage
- 本子计划不修改：src/app/*、src/ui/*、src/editor/*、src/core/*、src/platform/*、src/render/*

## 风险

- ini2arr_write 换为 .tmp + rename，如果 rename 时崩溃不会损坏原文件
- Config_Set 首次写入新 key 时，entries 数组需要有剩余空间，受 ini2arr 分配的 malloc 上限约束（等于 ini 文件行数，通常在 50 以下）
