# repair-5-a_debug_record — AppBar 死区残留问题调试记录

## 问题描述

在 EDGE_RESERVED（贴边占位）模式下，反复执行"进入→退出→再进入"操作时，偶尔出现如下现象：

- 退出贴边模式后，屏幕右侧仍保留一段 240px 左右的**死区**（任务栏工作区未恢复）
- 死区表现为：其他窗口最大化时无法进入该区域，桌面工作区被压缩
- 仅靠肉眼观察无法区分是该次退出失败还是上次进入未清理

**触发条件**：
- 贴边占位→退出→再贴边占位，循环 3-5 次左右必现
- **不移动窗口的情况下更容易触发**；移动窗口后很难复现

## 复现步骤（100% 可复现路径）

1. 启动 desknote，菜单→贴边占位（进入 EDGE_RESERVED）
2. 菜单→贴边占位（退出模式）
3. 菜单→贴边占位（再次进入）
4. 观察窗口右侧是否有原 240px 区域的死区残留

## 调试过程

### 阶段一：可疑点排查（已排除）

| 怀疑点 | 排查动作 | 结论 |
|--------|---------|------|
| state.ini 读取失败 | 在 `state_store.c` 加入 `[state_store_diag]` 诊断日志 | StateStore_Load 正常返回 0 |
| `resident_mode` 与 `AppBar_IsRegistered` 不同步 | DebugView 对比两者值 | 同步无异常 |
| app 层与 window 层异步命令队列时序 | 改用同步执行（`App_TryRegisterAppBarFromState` 直接调用，不走 `WM_TIMER`） | 未改善 |
| state.ini 字段 `release_on_hide_mode=0` 导致隐藏不释放 | `App_HideToTray` 改为**无条件释放 AppBar** | 未改善 |

### 阶段二：AppBar API 调优（已尝试）

| 尝试 | 改动 | 效果 |
|------|------|------|
| `AppBar_Unregister` 后调用 `SystemParametersInfoW(SPI_SETWORKAREA, ...)` 强制刷新工作区 | `appbar.c` | 未改善 |
| 退出贴边时 `MoveWindow` 把窗口移离右边缘 | `app.c` cmd103 退出分支 | 减轻频率但未根治 |
| 注销前先用 `ABM_SETPOS` 把 AppBar 劈到底部，再从底部移除 | `appbar.c` `AppBar_Unregister` | 减轻频率但未根治（当前状态） |

### 阶段三：根因推测（未确认）

当前最可能的根因：

1. **`SHAppBarMessage(ABM_REMOVE)` 偶尔不释放保留区**
   - 这是 Shell32 (Explorer) 的已知行为，非应用层可控
   - 在 Windows 多显示器、DPI 缩放、或 Explorer 意外重启后更容易出现

2. **`SystemParametersInfoW(SPI_GETWORKAREA)` 返回的工作区未及时反映 `ABM_REMOVE` 的结果**
   - 后续 `AppBar_Register` 读取到不正确的工作区

3. **窗口位置与保留区边界在像素级上不匹配**
   - 窗口正好在屏幕上某条边界对齐时，`ABM_NEW` / `ABM_SETPOS` 可能出现舍入误差
   - "移动窗口后很难触发"支持这一推测

## 当前代码状态

涉及改动的文件：

| 文件 | 改动 |
|------|------|
| `src/platform/win32/appbar.c` | `AppBar_Unregister` 先 SETPOS 到底部再 REMOVE + SPI_SETWORKAREA 刷新；`AppBar_Register` 增加 workarea 诊断日志 |
| `src/app/app.c` | cmd103 同步执行、退出后 MoveWindow 移离边缘；`App_RestoreFromTray` 释放后重置 resident_mode |
| `src/platform/win32/window.c` | `WM_COMMAND` handler 改为以 `App_GetResidentMode()` 为准 |

所有改动中无分层违规（app 层调用 appbar.h 公开接口，platform 层不调 app 层函数）。

## 后续修复方向（待选）

以下为可能的下一步方案，每个都需要独立验证：

1. **检测实际保留区**：在 `AppBar_Unregister` 后用 `AppBar_Register` + `AppBar_SetPosition` 主动重建，而不是依赖系统工作区的恢复。
2. **使用 `MonitorFromWindow` + `GetMonitorInfo`** 获取精确显示器工作区，代替 `SystemParametersInfoW`。
3. **等待 Shell-5b 实现 `TaskbarCreated` 消息处理**后，在 Explorer 刷新时自动重注册/重清理。
4. **在 `AppBar_Register` 和 `AppBar_Unregister` 中增加重试逻辑**（如反复调用 `ABM_REMOVE` 2-3 次，间隔 100ms）。
5. **使用 `DwmSetWindowAttribute` 或 `SetWindowLong` 主动去除窗口的 AppBar 风格属性**，从另一路径强制释放保留区。

## 关联计划

- `phase-10-shell-5a_repair-5-a-1` — 启动注册
- `phase-10-shell-5a_repair-5-a-3` — 浮动切换
- `phase-10-shell-5b` — 平台异常恢复（尚未实施，可能通过重注册缓解此问题）
