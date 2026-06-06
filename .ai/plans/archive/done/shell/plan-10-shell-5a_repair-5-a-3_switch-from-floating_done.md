# phase-10-shell-5a_repair-5-a-3 — 从浮动（Topmost）切换到 Edge：保存浮动边界并取消 Topmost

目标

- 在用户通过菜单/命令将窗口从浮动（FLOATING_TOPMOST）切换为 edge_reserved 时，保存当前浮动窗口的位置/大小到持久化（`last_floating_*`），取消 Topmost，然后调用平台注册 AppBar。

范围（单一小改动）

- 修改 `src/app/app.c` 中处理菜单命令（命令 ID 103）的分支，使之在切换路径上执行三件小事：保存浮动 bounds、取消 HWND 的 TOPMOST 属性、调用 `AppBar_Register`（通过 helper）。

实现步骤

1. 在菜单命令的 `case 103` 分支中检查当前 `shell_resident_mode` 是否为 `FLOATING_TOPMOST`（或等效状态标识）。
2. 如果是：
   - 获取当前窗口位置/大小（GetWindowRect / GetWindowPlacement），写入 `StateData.last_floating_*` 字段。
   - 立即调用 `SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)` 以清除 TOPMOST。
   - 持久化状态（调用 StateStore_Save）以保存 `last_floating_*`。
   - 调用 `App_TryRegisterAppBarFromState()`（或直接调用 `AppBar_Register`/`AppBar_SetPosition`），并记录日志（前缀 `[s5a-r5a3]`）。
3. 如果注册失败，记录并让 platform 重试；不要回滚 `last_floating_*`。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| D-1 | FLOATING_TOPMOST 切 EDGE_RESERVED | 窗口置顶在 (100,100) 位置 | 菜单→贴边占位 | 1) state.ini 中 last_floating_left=100, last_floating_top=100 2) 日志 `cmd103: saved last_floating_*` 3) 日志 `cmd103: cancelled topmost` 4) 窗口贴边 |

### 边界条件
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| D-2 | NONE 切 EDGE_RESERVED 不触发保存 | 窗口非置顶（NONE） | 菜单→贴边占位 | 无 saved 日志，无 cancelled 日志，直接贴边 |
| D-3 | EDGE_RESERVED 切回 NONE | 已贴边 | 菜单再次选择贴边占位 | 窗口取消贴边恢复普通 |

### 错误处理
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| D-4 | GetWindowRect 失败时跳过保存 | 模拟 GetWindowRect 返回 FALSE | 置顶模式切贴边 | 日志无 saved 日志，取消 topmost 仍执行，贴边仍尝试注册 |

### 回归
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| D-5 | 浮动置顶到贴边再切回浮动 | D-1 执行后，菜单再次选择浮动置顶 | 菜单→浮动置顶 | 窗口恢复到 last_floating_* 位置并置顶 |
| D-6 | 编译通过 | 代码已修改 | `cmake --build build` | 零错误零警告 |

### 集成
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| D-7 | 浮动→贴边→隐藏→恢复→贴边 | D-1 后隐藏，恢复，再菜单贴边 | 完整路径 | 每一步都有对应日志，最终贴边一致 |

依赖

- 依赖 repair-5-a-5（参数校验和日志工具）以确保一致性；依赖 5b（见 `phase-10-shell-5b.md`）用于后续平台恢复。

预计耗时

- 30–60 分钟（实现 + 手动验证）。
