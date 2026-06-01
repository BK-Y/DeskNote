# Shell-5a_repair-5 — 补全贴边模式恢复路径（后续）

## ① 核心问题

Shell-5a 基本路径实现了"菜单切换贴边↔取消贴边"，但贴边模式无法在以下场景中自动恢复：
1. 贴边状态下退出程序 → 重启后窗口不贴边
2. 贴边状态下隐藏到托盘 → 恢复后 AppBar 丢失，窗口变普通模式
3. 从浮动置顶直接切贴边 → topmost 未被取消，窗口同时置顶且注册 AppBar

## ② 分析现状

### 阶段摘要

从"贴边只在手动切换时有效"推进到"贴边在重启恢复 / 托盘恢复 / 模式互切时均自动保持"。

### 承接关系

前置依赖：

- **Shell-5a_done**（已回滚归档）：
  - `appbar.h/c`：Register / Unregister / SetPosition / IsRegistered 均可正常调用
  - `window.c`：`ENTER_EDGE_RESERVED` 命令收束分支已就绪
  - `app.c`：菜单 cmd==103 切换逻辑已就绪（含 `resident_mode` 更新 + dock 配置持久化）
  - `App_Init`：已实现 floating_topmost 启动恢复，缺少 edge_reserved 分支
  - `App_RestoreFromTray`：已实现 floating_topmost 托盘恢复，缺少 edge_reserved 分支
  - repair-2 枚举修正 + repair-3 诊断输出已保留

### 三处代码缺口的精确位置

**缺口 1：`App_Init` 末尾（app.c L822-839）**

```c
/* Shell-4a_2: 启动时恢复浮动置顶状态 */
{
    StateData state;
    StateStore_Load(&state);
    if (state.shell_resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
    {
        SetWindowPos(hwnd, HWND_TOPMOST,
                    state.last_floating_left, state.last_floating_top,
                    state.last_floating_width, state.last_floating_height,
                    SWP_NOZORDER);
    }
}
// ← 缺少 EDGE_RESERVED 分支
```

**缺口 2：`App_RestoreFromTray` 末尾（app.c L1410-1413）**

```c
/* Shell-4a_1: 恢复 topmost */
if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
{
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
// ← 缺少 EDGE_RESERVED 分支
```

**缺口 3：`cmd == 103` 分支 `else` 块开头（app.c L1211-1215）**

```c
else
{
    // ← 缺少：从 floating_topmost 切到 edge_reserved 时取消 topmost
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;
    ...
}
```

## ③ 方案设计

**唯一路径**：三处均为已有条件判断中漏掉了 `EDGE_RESERVED` 分支，直接在现有代码块内补全。不改接口、不改分层、不改其他文件。

> 被排除的方案：新增独立状态恢复模块。理由：恢复逻辑紧贴现有 if-else 分支，拆出独立模块反而增加跨层调用复杂度，且 5a 本身已归档，本 repair 是补漏而非重构。

## ④ 拆解执行

### 整体目标

1. 贴边后退出重启 → 自动恢复贴边状态
2. 贴边后隐藏到托盘再恢复 → AppBar 自动重建
3. 浮动置顶切贴边 → 先取消 topmost 再注册 AppBar

### 阶段产出

1. 重启后窗口自动贴边（如果上次退出时处于贴边模式）
2. 从托盘恢复时窗口自动重建 AppBar
3. 从浮动置顶切到贴边时窗口先取消置顶

## ⑤ 设定边界

### 本阶段范围

三处 app.c 中的代码补全。每处均调 `appbar.h` 已声明的公开接口，不跨层。

### 不包括

1. 不做隐藏到托盘时释放 AppBar（归 5b）
2. 不做 300px 死区修复（独立 repair）
3. 不做异常恢复路径（归 5b）

### 分层归属

- **app/** — `App_Init`、`App_RestoreFromTray`、菜单命令处理中的恢复逻辑
- **platform/appbar** — 被调方，接口已就绪，本阶段不改

### 文件落点

#### 预计修改文件

- `src/app/app.c` — 3 处新增代码块

#### 原则上不应修改

- `src/platform/win32/appbar.h`、`src/platform/win32/appbar.c`
- `src/platform/win32/window.c`
- `src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`、`src/ui/*`

## ⑥ 落地方案

### 修复 1：App_Init — 启动恢复 edge_reserved

在现有 `Shell-4a_2` 块之后、`App_EnsureCaretVisible()` 之前插入：

```c
/* Shell-5a_repair-5: 启动时恢复贴边占位状态 */
{
    StateData state;
    StateStore_Load(&state);
    if (state.shell_resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
    {
        AppDockEdge edge = (AppDockEdge)state.dock_edge;
        int thickness = state.dock_thickness;
        if (AppBar_Register(hwnd) == 0)
        {
            AppBar_SetPosition(hwnd, edge, thickness);
            g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;
        }
    }
}
```

`g_app.shell.resident_mode` 由 `App_ApplyShellStateData` 的 switch 中 `EDGE_RESERVED` case 设置（Shell-4 修复时已补全）。如果 `Register` 失败则 `resident_mode` 不更新，下次尝试时仍会进入此分支。

### 修复 2：App_RestoreFromTray — 从托盘恢复 AppBar

在现有 `Shell-4a_1` 块之后追加：

```c
/* Shell-5a_repair-5: 从托盘恢复贴边占位 */
if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
{
    StateData state;
    StateStore_Load(&state);
    AppDockEdge edge = (AppDockEdge)state.dock_edge;
    int thickness = state.dock_thickness;
    if (AppBar_Register(hwnd) == 0)
        AppBar_SetPosition(hwnd, edge, thickness);
}
```

此处不需要再设 `g_app.shell.resident_mode`——`App_RestoreFromTray` 进入时 `resident_mode` 已经是 `EDGE_RESERVED`（`App_HideToTray` 不改 `resident_mode`）。使用独立变量 `state` 避免与上方 3c_2 持久化块的 `state` 作用域混淆。

### 修复 3：cmd==103 — 浮动置顶切贴边前取消 topmost

在 `else` 块内、`resident_mode = EDGE_RESERVED` 之前插入：

```c
else
{
    /* Shell-5a_repair-5: 从浮动置顶切到贴边时先取消 topmost */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
    {
        SetWindowPos(g_app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE);
    }

    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;
    ...
}
```

`SetWindowPos(HWND_NOTOPMOST, ..., SWP_NOMOVE | SWP_NOSIZE)` 只移除 topmost 样式，不改窗口位置和尺寸。后续 `App_SubmitShellCommand(ENTER_EDGE_RESERVED)` → `window.c` 执行 `AppBar_SetPosition` 时 `MoveWindow` 会把窗口移到贴边位置。

### 主链路

```text
路径 A：贴边→退出→重启
    启动 → App_Init → 读 state.ini → EDGE_RESERVED 分支
    → AppBar_Register → AppBar_SetPosition → MoveWindow 贴边

路径 B：贴边→隐藏到托盘→恢复
    托盘左键 → WM_TRAYICON → WM_TIMER → App_RestoreFromTray
    → EDGE_RESERVED 分支 → AppBar_Register → AppBar_SetPosition

路径 C：浮动置顶→菜单→贴边
    cmd==103 → FLOATING_TOPMOST 分支
    → SetWindowPos(HWND_NOTOPMOST) → resident_mode = EDGE_RESERVED
    → 持久化 → ENTER_EDGE_RESERVED → AppBar_Register → SetPosition
```

## ⑦ 验收标准

1. 贴边状态下退出程序，重启后窗口自动贴边到相同边（右，240px）
2. 贴边状态下隐藏到托盘，用托盘左键恢复后 AppBar 重建，窗口仍贴边
3. 浮动置顶状态下点击菜单"贴边占位"，窗口取消置顶后贴边
4. 编译通过

5. 分层核实：
   - 三处修复均位于 `app.c`，仅调 `appbar.h` 公开接口（`AppBar_Register` / `AppBar_SetPosition`），不直接访问 `s_appbar` 内部字段，不调 platform 层其他函数
   - `appbar.c`、`window.c`、`appbar.h` 本阶段零改动

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `App_Init` 末尾、`Shell-4a_2` 块之后、`App_EnsureCaretVisible()` 之前插入 `Shell-5a_repair-5` 恢复块——读 `state.ini` 的 `shell_resident_mode`，若为 `EDGE_RESERVED` 则读取 `dock_edge` + `dock_thickness`，调 `AppBar_Register` + `AppBar_SetPosition`，成功后设 `g_app.shell.resident_mode = EDGE_RESERVED` | 启动时自动恢复贴边 | 编译通过 |
| 2 | `App_RestoreFromTray` 中 `Shell-4a_1` 块之后追加 `Shell-5a_repair-5` 恢复块——检查 `g_app.shell.resident_mode == EDGE_RESERVED`，从 `state.ini` 读取 dock 配置后调 `AppBar_Register` + `AppBar_SetPosition` | 托盘恢复时重建 AppBar | 编译通过 |
| 3 | `cmd==103` 的 `else` 块中，在 `resident_mode = EDGE_RESERVED` 之前插入 `Shell-5a_repair-5` 判断——若当前 `resident_mode == FLOATING_TOPMOST`，先调 `SetWindowPos(HWND_NOTOPMOST, ..., SWP_NOMOVE \| SWP_NOSIZE)` 取消置顶 | 浮动置顶切贴边时先退置顶 | 编译通过 |
| 4 | 验证路径 A：贴边 → 退出 → 重启 → 窗口自动贴边 | 恢复正确 | 手动 |
| 5 | 验证路径 B：贴边 → 隐藏到托盘 → 托盘恢复 → 窗口仍贴边 | 恢复正确 | 手动 |
| 6 | 验证路径 C：浮动置顶 → 点击"贴边占位" → 窗口先退置顶再贴边 | 切换正确 | 手动 |
