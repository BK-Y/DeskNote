# Shell-3c_1_repair-1 — 修复标题栏菜单"关闭"未改为隐藏（后续）

## ① 核心问题

Shell-3c_1 已标记完成，但 `src/app/app.c` 中标题栏汉堡菜单的"关闭"按钮（`cmd == 100`）仍然是 `PostQuitMessage(0)`，没有改为 `App_HideToTray`。导致点汉堡菜单"关闭"仍然直接退出程序，而不是隐藏到托盘。

## ② 分析现状

### 阶段摘要

修复 3c_1 中遗漏的 `cmd == 100` 分支改动：从 `PostQuitMessage(0)` 改为 `App_HideToTray(hwnd)`。

### 承接关系

前置阶段：Shell-3c_1_done（已标记完成，但该分支被遗漏）

## ③ 方案设计

只有一条路径：改 `App_SubmitShellCommand` 中 `cmd == 100` 的分支逻辑。不涉及多方案比选。

## ④ 拆解执行

### 整体目标

标题栏汉堡菜单"关闭" → 隐藏到托盘，不退出。

### 阶段产出

点标题栏汉堡菜单"关闭"，窗口隐藏到托盘（不退出）。

## ⑤ 设定边界

### 本阶段范围

只改 app.c 中 cmd==100 这一行，不改其他任何代码。

### 不包括

1. 不做 WM_CLOSE 改动（已在 3c_1 完成）
2. 不做其他关闭路径的调整
3. 不做状态持久化

### 分层归属

- **app/** — 改 `App_SubmitShellCommand` 中 `cmd == 100` 分支

### 文件落点

#### 预计修改文件

- `src/app/app.c` — `cmd == 100` 行

#### 原则上不应修改

- 所有其他文件

## ⑥ 落地方案

### 技术路线

**app.c（标题栏菜单"关闭"命令分发）：**

```c
if (cmd == 100) /* 关闭 → 改为隐藏到托盘（退出唯一入口在托盘菜单） */
    App_HideToTray(g_app.hwnd);
else if (cmd == 101) /* 关于 */
    MessageBoxW(...);
```

### 主链路

```text
用户点标题栏汉堡菜单"关闭"
-> App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_MENU)
-> cmd == 100
-> App_HideToTray(hwnd) → ShowWindow(SW_HIDE) → 窗口隐藏到托盘
```

## ⑦ 验收标准

1. 点标题栏汉堡菜单"关闭" → 窗口隐藏到托盘，不退出
2. 除改的这一行代码外，没有其他文件被修改
3. 编译通过，0 error 0 warning

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/app/app.c` 的 `App_SubmitShellCommand` 中，将 `if (cmd == 100)` 分支的 `PostQuitMessage(0)` 改为 `App_HideToTray(g_app.hwnd)`——标题栏汉堡菜单的关闭行为统一走隐藏逻辑 | 汉堡菜单"关闭"不再退出，改为隐藏 | 编译通过 |
| 2 | 验证：点标题栏汉堡菜单"关闭"，窗口隐藏到托盘不退出 | 关闭行为正确 | 手动验证 |
