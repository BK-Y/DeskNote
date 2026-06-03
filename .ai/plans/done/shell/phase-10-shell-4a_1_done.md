# Shell-4a_1 — floating topmost 状态机（后续）

## ① 核心问题

便签窗口只能普通显示，切到其他应用就被盖住。用户想要"浮在最前"但做不到。

## ② 分析现状

### 阶段摘要

从"没有置顶能力"推进到"标题栏菜单可切换浮动置顶，隐藏恢复后保持"。

### 承接关系

前置阶段：Shell-3c_2（状态持久化已就绪）
前置阶段：Shell-3b（App_HideToTray / App_RestoreFromTray 已就绪）

## ③ 方案设计

只有一条合理路径：由 app 层收束 TOPMOST 切换命令，通过现有命令管线执行 SetWindowPos。不走 titlebar 直接调——置顶切换虽然代码简单，但它属于壳层模式管理，应统一走 app 决策层，与 Shell-3b/3c 的模式一致。

## ④ 拆解执行

### 整体目标

1. AppShellResidentMode 新增 FLOATING_TOPMOST
2. APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST 走命令管线，app 层收束执行 SetWindowPos
3. App_RestoreFromTray 中检查 resident_mode 恢复 topmost

### 阶段产出

1. 标题栏菜单可切换浮动置顶
2. 置顶后窗口浮在其他窗口之上
3. 取消置顶后恢复正常
4. 隐藏→恢复后置顶保持

## ⑤ 设定边界

### 本阶段范围

只做 topmost 状态切换。不做 bounds 持久化（Shell-4a_2）。

### 不包括

1. 不做窗口位置保存
2. 不做启动恢复
3. 不做 AppBar

### 分层归属

- **app/** — 收束 TOGGLE_FLOATING_TOPMOST 命令，执行 SetWindowPos 并更新 resident_mode；App_RestoreFromTray 中恢复 topmost
- **ui/titlebar** — 菜单项提交命令到 app
- **platform/** — 不新增职责

### 文件落点

#### 预计修改文件

- `src/app/app.h` — AppShellResidentMode 新增 FLOATING_TOPMOST
- `src/app/app.c` — 命令收束分支 + App_RestoreFromTray 恢复逻辑
- `src/ui/titlebar.c` — 菜单项提交命令到 app

#### 原则上不应修改

- `src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`、`src/platform/win32/window.c`

## ⑥ 落地方案

### 技术路线

**app.h：**

```c
typedef enum {
    APP_SHELL_RESIDENT_MODE_NONE = 0,
    APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST
} AppShellResidentMode;
```

**window.c（命令收束 switch 中新增分支）：**

在 WM_TIMER 的 `App_TakePendingShellCommand` 收束 switch 中，`APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST` 分支。window.c 是平台层，在此处调 SetWindowPos 符合架构约束：

```c
case APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST:
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
    {
        SetWindowPos(g_app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
    }
    else
    {
        SetWindowPos(g_app.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
    }
    break;
```

**app.c（App_RestoreFromTray 中恢复 topmost）：**

在持久化逻辑后插入：

```c
    /* Shell-4a_1: 恢复 topmost（如果之前是置顶模式） */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
```

**titlebar.c：**

菜单项命令 ID 对应 `APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST`，通过 `App_SubmitShellCommand` 提交。该命令已在 `App_GetShellCommandGroupInternal` 中映射到 `SHELL_MODES` 组，命令组已包含该组权限，直接可用。

### 主链路

```text
用户点击"浮动置顶"
-> titlebar 菜单 → App_SubmitShellCommand(TOGGLE_FLOATING_TOPMOST)
-> WM_TIMER 收束 → App_TakePendingShellCommand
-> switch 分支
-> 如果 NONE → SetWindowPos(HWND_TOPMOST)，mode = FLOATING_TOPMOST
-> 如果 FLOATING_TOPMOST → SetWindowPos(HWND_NOTOPMOST)，mode = NONE

从托盘恢复
-> App_RestoreFromTray
-> 检查 resident_mode
-> 如果是 FLOATING_TOPMOST → SetWindowPos(HWND_TOPMOST)
```

## ⑦ 验收标准

1. 标题栏菜单出现"浮动置顶"切换项
2. 切换到置顶后，其他窗口不覆盖便签
3. 取消置顶后恢复正常
4. 隐藏→恢复后置顶保持
5. 编译通过，分层归属与文件落点一致

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/app/app.h` 的 `AppShellResidentMode` 枚举新增 `APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST`——扩展常驻模式枚举 | 枚举就绪 | 编译通过 |
| 2 | `src/platform/win32/window.c` 的 WM_TIMER 命令收束 switch 中新增 `TOGGLE_FLOATING_TOPMOST` 分支，检查 mode 并 SetWindowPos 切换——window.c 是平台层，在此处调系统 API 符合架构约束 | 置顶/取消置顶切换逻辑就绪 | 编译通过 |
| 3 | `src/app/app.c` 的 `App_RestoreFromTray` 中插入检查 resident_mode 恢复 topmost——隐藏→恢复时保持置顶状态；此处仅做状态判断，不直接调系统 API，SetWindowPos 在 window.c 中调 | 恢复时正确应用 topmost | 编译通过 |
| 4 | `src/ui/titlebar.c` 的标题栏菜单中新增"浮动置顶"菜单项，点击时提交 `APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST` 到 app——菜单项只负责提交命令，不执行业务逻辑 | 菜单项出现 | 编译通过 |
| 5 | 验证：切换置顶，其他窗口不覆盖便签 | 置顶行为正确 | 手动验证 |
| 6 | 验证：取消置顶，恢复正常 | 取消正确 | 手动验证 |
| 7 | 验证：置顶后隐藏→恢复，置顶保持 | 保持正确 | 手动验证 |
