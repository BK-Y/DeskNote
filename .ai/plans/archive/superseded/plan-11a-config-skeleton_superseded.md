# Plan-11a — 模块骨架

## 核心问题

config 模块尚无独立文件，接口和实现耦合在 11b 规划中，无法提前编译验证接口设计是否正确。

## 分析现状

- 前置依赖：`src/utils/ini2arr.h` 已存在
- 现状：config 模块还未创建，API 接口只存在于 plan-11 总纲的文字描述中
- 产出：Plan-11a 完成后 config.h + config.c 存在，头文件语法可通过编译器验证

## 方案选型

- **方案 A（选中）：独立骨架文件**
  - 创建 `src/config/config.h + config.c`，定义 5 个 API 的空实现
  - `app.c` 可 `#include "../config/config.h"`，语法编译通过
  - 符合分层规则：config 层仅依赖 utils，不依赖 app/platform/editor/ui/render/storage

- **方案 B（排除）：骨架合入 11b 不单独建文件**
  - 11a 不做，等 11b 直接建完整实现
  - 排除理由：11b 需要先有 config.h 才能写 Config_Init 的实现体；没有提前的接口定义，无法在 11b 之前确认 API 签名是否合理

## 拆解执行

### 主链路

```
创建目录 → 写 config.h → 写 config.c（空实现）→ 语法验证
```

## 设定边界

### 范围

- 创建 `src/config/config.h` 和 `src/config/config.c`
- 只写接口声明和空函数体（所有函数 return 0 或空操作）
- 从 `app.c` 临时加 `#include` 验证头文件语法正确

### 不做

- 不接入 ini2arr（11b 做）
- 不修改任何现有代码的逻辑
- 不新建测试文件（11b 一步到测试）
- CMakeLists.txt 不改（11b 加源文件）

### 不修改的文件

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*、CMakeLists.txt

## 落地方案

### 接口说明

```c
#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_KEY_LEN 63

typedef void (*ConfigCallback)(const char* key, int old_val, int new_val);

int  Config_Init(const char* path);
int  Config_Get(const char* key, int default_val);
int  Config_Set(const char* key, int new_val);
void Config_OnChange(ConfigCallback cb);
void Config_Shutdown(void);

#endif
```

### 骨架实现

```c
#include "config.h"

static ConfigCallback g_cb = NULL;

int Config_Init(const char* path) { (void)path; return 0; }
int Config_Get(const char* key, int default_val) { (void)key; return default_val; }
int Config_Set(const char* key, int new_val) { (void)key; (void)new_val; return 0; }
void Config_OnChange(ConfigCallback cb) { g_cb = cb; }
void Config_Shutdown(void) {}
```

## 验收标准

- [ ] `src/config/config.h` 存在，包含 5 个 API 声明
- [ ] `src/config/config.c` 存在，包含 5 个空实现
- [ ] `gcc -fsyntax-only src/config/config.c` 无报错
- [ ] `app.c` 顶部 `#include "../config/config.h"` 通过语法检查
- [ ] 分层核实：config.h 不 #include 任何业务模块头文件

## 推进步骤

### 方案 A：独立骨架文件 [selected]

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | 创建 `src/config/` 目录 | 目录存在 | `ls src/config/` |
| 2 | 写入 `src/config/config.h`（5 个函数声明） | 头文件存在，语法正确 | `gcc -fsyntax-only src/config/config.h` |
| 3 | 写入 `src/config/config.c`（5 个空实现） | 源文件存在，语法正确 | `gcc -fsyntax-only src/config/config.c` |
| 4 | `app.c` 顶部加 `#include "../config/config.h"` | 头文件可被其他文件引用 | `gcc -fsyntax-only src/app/app.c` |
| 5 | 恢复 app.c（本步只是验证语法，完成后移除该 include） | app.c 恢复原状 | `git diff src/app/app.c` 确认无改动 |

### 方案 B：合入 11b（备选）

（本方案未选中，推进步骤为空）

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [agent] `test/test_config.c` 存在
- [agent] `src/config/config.h` 和 `src/config/config.c` 存在
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 头文件存在性 | `ls src/config/config.h src/config/config.c` | 两个文件都存在 |
| A-2 | 分层隔离：config.h 不含业务层依赖 | `grep -cE '#include.*(app\|platform\|editor\|ui\|render\|storage)' src/config/config.h` | 输出为 0 |
| A-3 | 单元测试全部通过 | `gcc -o test_config test/test_config.c src/config/config.c src/utils/ini2arr.c && ./test_config` | 10/10 PASS |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 |
|------|---------|---------|
| M-1 | 1. 编辑 state.ini 设 `shell_resident_mode=2`<br>2. 启动应用 | 窗口贴右边缘，AppBar 注册 |
| M-2 | 1. 启动应用<br>2. 菜单→贴边占位 | 窗口贴右边缘，state.ini 中 `shell_resident_mode=2` |
| M-3 | 1. 浮动模式下拖拽窗口<br>2. 关闭→重启 | 窗口回到拖拽前记录的浮动位置 |

### GATE 1-2 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
