# Plan-11a — 模块骨架

## 目标
创建 `src/config/config.c` / `config.h`，定义 API 接口，编译通过。

## 步骤

### 1. 创建目录

```
src/config/
```

### 2. 创建 config.h

```c
#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_KEY_LEN 63

typedef void (*ConfigCallback)(const char* key, int old_val, int new_val);

int  Config_Init(const char* path);
int  Config_Get(const char* key, int default_val);
int  Config_Set(const char* key, int new_val);
void Config_OnChange(ConfigCallback cb);
int  Config_Shutdown(void);

#endif
```

### 3. 创建 config.c（骨架实现）

```c
#include "config.h"

static ConfigCallback g_cb = NULL;

int Config_Init(const char* path) { (void)path; return 0; }
int Config_Get(const char* key, int default_val) { (void)key; return default_val; }
int Config_Set(const char* key, int new_val) { (void)key; (void)new_val; return 0; }
void Config_OnChange(ConfigCallback cb) { g_cb = cb; }
int Config_Shutdown(void) { return 0; }
```

### 4. 验证编译

CMakeLists.txt 暂不改动。头文件放在正确位置即可，app.c 顶部加 `#include "../config/config.h"` 编译通过。

## 不必

- 不必接入 ini2arr（11b 做）
- 不必修改现有代码逻辑
- 不必新建测试文件（11b 一步到测试）

## 分层核实
