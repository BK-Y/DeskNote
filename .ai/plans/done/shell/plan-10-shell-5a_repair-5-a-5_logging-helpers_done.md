# phase-10-shell-5a_repair-5-a-5 — 日志前缀与参数校验（小工具）

目标

- 添加一组小的通用改进：统一的调试日志前缀/宏，以及用于校验 `dock_edge` 和 `dock_thickness` 的 helper。其它 repair-5-a 子计划应依赖这些工具以保证一致性。

范围（单一小改动）

- 在 `src/app/app.c`（或 `src/app/logging.h` 若已有公共头）新增：
  - 一个常量或宏作为日志前缀，例如 `#define S5A_LOG_PREFIX L"[s5a-r5a] "`。
  - 一个 `static int ClampDockEdge(int edge)`（返回 0..3）和 `static int ClampDockThickness(int thickness)`（返回 1..1024 或与项目中相符的合理范围）。

实现步骤

1. 确认项目现有的日志风格；在保留现有风格的前提下标准化用于 repair-5-a 系列的前缀。
2. 在 `src/app/app.c` 添加上述 helper（或放到 `app_helpers.c` 若团队偏好）。
3. 在 repair-5-a-1 / repair-5-a-3 等计划中使用这些 helper 来避免重复实现。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| A-1 | ClampDockEdge 合法值 | 代码已编译 | 代码审查 `ClampDockEdge(0)` ~ `ClampDockEdge(3)` | 返回原值不变 |
| A-2 | ClampDockThickness 边界值 | 代码已编译 | 代码审查 `ClampDockThickness(200)`、`ClampDockThickness(500)` | 返回 200 和 500 |

### 边界条件
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| A-3 | ClampDockEdge 非法值 | 代码已编译 | 代码审查 `ClampDockEdge(-1)`、`ClampDockEdge(4)`、`ClampDockEdge(99)` | 全部返回 `APP_DOCK_RIGHT(2)` |
| A-4 | ClampDockThickness 钳位 | 代码已编译 | 代码审查 `ClampDockThickness(0)`、`ClampDockThickness(50)`、`ClampDockThickness(999)` | 钳位到 200 和 500 |

### 错误处理（不适用，本计划为纯工具函数，无运行时失败路径）

### 回归
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| A-5 | 编译不产生新警告 | 代码已修改 | `cmake --build build` | 零错误零警告 |

### 集成
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| A-6 | 日志前缀在 DebugView 中可见 | 程序启动，DebugView 已打开 | 运行 desknote（正常模式） | DebugView 中过滤 `[s5a-r5a]` 无输出（因未进 EDGE_RESERVED 路径，但宏已编译通过） |

依赖与注意事项

- 10–20 分钟。
