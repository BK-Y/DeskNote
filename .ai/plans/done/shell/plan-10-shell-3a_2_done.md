# Shell-3a_2 — 托盘消息管线（后续）

## ① 核心问题

托盘图标创建了，但点击它没有任何反应——没有事件处理，无法通过托盘恢复窗口或弹出菜单。

## ② 分析现状

### 阶段摘要

从"托盘图标只是摆在那里"推进到"左键点击恢复窗口、右键弹出菜单（显示/退出）"。

### 承接关系

前置阶段：Shell-3a_1（托盘图标已创建，App_InitTrayIcon / App_DestroyTrayIcon 已就绪）

## ③ 方案设计

**方案 A：WndProc 直接处理全部逻辑**
- platform 收 WM_TRAYICON → 直接 ShowWindow / CreatePopupMenu，不经过 app
- 优点：代码最短，一个 case 写完
- 缺点：后期 3b 重构时要改已完成的 window.c（违反已完成阶段冻结）
- 架构合规：不符合——违反"app 负责收束壳层模式切换"的约定

**方案 B：独立消息循环线程**
- 单独开一个线程监听托盘事件，通过自定义消息通知主线程
- 缺点：Win32 托盘消息本身就是发到窗口过程的，额外线程无意义且增加复杂度
- 架构合规：过度设计

**方案 C：app 层收束托盘命令 [selected]**
- platform（WndProc）只做消息翻译：
  - WM_LBUTTONUP → 提交 `APP_SHELL_COMMAND_RESTORE_FROM_TRAY`
  - WM_RBUTTONUP → 提交 `APP_SHELL_COMMAND_SHOW_TRAY_MENU`
- app 层做实际执行：App_TakePendingShellCommand 分发中处理两个新命令
- 优点：
  - 分层正确——platform 只翻译系统事件，app 做决策和执行
  - 3b 阶段只需要改 app.c 中 RESTORE_FROM_TRAY 的实现，不需要动已完成的 window.c
  - 3c_1 不依赖 3a_2 的任何内容，完全解耦
- 架构合规：符合——"app 负责收束壳层模式切换"

## ④ 拆解执行

### 整体目标

1. 处理 WM_TRAYICON 消息，左键/右键分别提交对应命令到 app
2. app 收到 RESTORE_FROM_TRAY 命令后恢复窗口到前台
3. app 收到 SHOW_TRAY_MENU 命令后弹出托盘菜单（显示/退出）
4. 菜单项"显示"恢复窗口，"退出"终止程序

### 阶段产出

1. 左键点击托盘图标 → 窗口恢复到前台
2. 右键点击托盘图标 → 弹出菜单（"显示窗口"、"退出"）
3. 点击"显示窗口" → 窗口恢复到前台
4. 点击"退出" → 程序终止
5. platform 和 app 分层正确，3b 不需要改 window.c

## ⑤ 设定边界

### 本阶段范围

只做托盘的事件处理和菜单弹出。platform 只做消息转发，执行在 app 层。

### 不包括

1. 不做 presence 状态管理（那是 Shell-3b，3b 将增强 RESTORE_FROM_TRAY 的实现）
2. 不做关闭策略区分（那是 Shell-3c_1）
3. 不做状态持久化（那是 Shell-3c_2）
4. 不做标题栏菜单改造

### 分层归属

- **platform/win32/window** — WndProc 中处理 WM_TRAYICON：左键提交 RESTORE_FROM_TRAY，右键提交 SHOW_TRAY_MENU。弹出菜单由 app 触发，平台层执行 TrackPopupMenu
- **app/** — App_TakePendingShellCommand 分发中处理两个新命令

### 文件落点

#### 预计修改文件

- `src/app/app.h` — 新增 APP_SHELL_COMMAND_RESTORE_FROM_TRAY、APP_SHELL_COMMAND_SHOW_TRAY_MENU 枚举值
- `src/app/app.c` — 处理两个新命令的分发逻辑
- `src/platform/win32/window.c` — WM_TRAYICON case 分支改为提交命令到 app

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`

## ⑥ 落地方案

### 技术路线

**app.h 新增枚举值：**

```c
typedef enum {
    APP_SHELL_COMMAND_NONE = 0,
    APP_SHELL_COMMAND_SHOW_MENU,
    APP_SHELL_COMMAND_WINDOW_CLOSE,
    APP_SHELL_COMMAND_WINDOW_RESTORE,
    APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST,
    APP_SHELL_COMMAND_ENTER_EDGE_RESERVED,
    APP_SHELL_COMMAND_HIDE_TO_TRAY,
    APP_SHELL_COMMAND_RESTORE_FROM_TRAY,   /* Shell-3a_2 */
    APP_SHELL_COMMAND_SHOW_TRAY_MENU       /* Shell-3a_2 */
} AppShellCommand;
```

**App_GetShellCommandGroupInternal 增加映射：**

```c
case APP_SHELL_COMMAND_SHOW_TRAY_MENU:
case APP_SHELL_COMMAND_RESTORE_FROM_TRAY:
    *out_group = APP_TITLEBAR_COMMAND_GROUP_BACKGROUND_ACTIONS;
    return 0;
```

**window.c（WM_TRAYICON — 只做翻译，不做执行）：**

```c
case WM_TRAYICON:
    if (lParam == WM_LBUTTONUP)
        App_SubmitShellCommand(APP_SHELL_COMMAND_RESTORE_FROM_TRAY);
    else if (lParam == WM_RBUTTONUP)
        App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_TRAY_MENU);
    return 0;
```

**window.c（WM_COMMAND — 处理托盘菜单项点击）：**

```c
case WM_COMMAND:
    switch (LOWORD(wParam))
    {
        case 1001: /* 托盘菜单：显示窗口 */
            App_SubmitShellCommand(APP_SHELL_COMMAND_RESTORE_FROM_TRAY);
            break;
        case 1002: /* 托盘菜单：退出 */
            DestroyWindow(hwnd);
            break;
    }
    return 0;
```

**window.c（WM_TIMER 命令收束 — 在现有自动保存定时器中检查待处理命令）：**

```c
case WM_TIMER:
    if (wParam == WINDOW_AUTOSAVE_TIMER_ID)
    {
        /* Shell-3a_2: 在处理自动保存前，先收束待处理的托盘命令 */
        AppShellCommand cmd;
        if (App_TakePendingShellCommand(&cmd) == 0)
        {
            switch (cmd)
            {
                case APP_SHELL_COMMAND_RESTORE_FROM_TRAY:
                    ShowWindow(hwnd, SW_SHOW);
                    SetForegroundWindow(hwnd);
                    break;
                case APP_SHELL_COMMAND_SHOW_TRAY_MENU:
                    Platform_ShowTrayMenu(hwnd);
                    break;
                default:
                    break;
            }
        }

        App_OnTick();
    }
    return 0;
```

**window.c 新增 Platform_ShowTrayMenu：**

```c
void Platform_ShowTrayMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1001, L"显示窗口");
    AppendMenuW(hMenu, MF_STRING, 1002, L"退出");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}
```

**window.h 声明 Platform_ShowTrayMenu：**

```c
void Platform_ShowTrayMenu(HWND hwnd);
```

### 主链路

```text
用户左键托盘图标
-> WM_TRAYICON → lParam == WM_LBUTTONUP
-> App_SubmitShellCommand(APP_SHELL_COMMAND_RESTORE_FROM_TRAY)
-> window.c 收束命令 → ShowWindow(SW_SHOW) + SetForegroundWindow
-> 窗口恢复前台

用户右键托盘图标
-> WM_TRAYICON → lParam == WM_RBUTTONUP
-> App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_TRAY_MENU)
-> window.c 收束命令 → Platform_ShowTrayMenu(hwnd)
-> TrackPopupMenu 弹出菜单
-> 用户点击"显示窗口" → WM_COMMAND 1001 → 再次提交 RESTORE_FROM_TRAY
-> 用户点击"退出" → WM_COMMAND 1002 → DestroyWindow
```

### 接口说明

```c
/* platform 层：弹出托盘菜单 */
void Platform_ShowTrayMenu(HWND hwnd);
```

## ⑦ 验收标准

1. 左键点击托盘图标，窗口恢复到前台
2. 右键点击托盘图标，弹出菜单（显示窗口/退出）
3. 菜单"显示窗口"恢复窗口
4. 菜单"退出"终止程序
5. platform 只做消息翻译，不直接执行业务逻辑
6. 分层归属与文件落点一致

## ⑧ 推进步骤

### 方案 C：app 层收束托盘命令 [selected]

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | `src/app/app.h` 枚举新增 `APP_SHELL_COMMAND_RESTORE_FROM_TRAY` 和 `APP_SHELL_COMMAND_SHOW_TRAY_MENU`；`src/platform/win32/window.h` 声明 `Platform_ShowTrayMenu(HWND)` | 新枚举值和函数声明就绪 | 编译通过 |
| 2 | `src/app/app.c` 的 `App_GetShellCommandGroupInternal` 中增加两个新命令到 `BACKGROUND_ACTIONS` 组 | 命令组映射就绪 | 编译通过 |
| 3 | `src/platform/win32/window.c` 新增 `Platform_ShowTrayMenu(HWND)` 函数实现（CreatePopupMenu + AppendMenuW + TrackPopupMenu） | 托盘菜单弹出函数就绪 | 编译通过 |
| 4 | `src/platform/win32/window.c` 的 WndProc 中新增 `case WM_TRAYICON:` 分支，左键提交 `RESTORE_FROM_TRAY`，右键提交 `SHOW_TRAY_MENU` | platform 只做消息翻译 | 编译通过 |
| 5 | `src/platform/win32/window.c` 的 `case WM_TIMER:` 中，`App_OnTick` 调用前插入 `App_TakePendingShellCommand` 收束逻辑，处理 `RESTORE_FROM_TRAY`（ShowWindow + SetForegroundWindow）和 `SHOW_TRAY_MENU`（Platform_ShowTrayMenu） | 命令通过定时器收束 | 编译通过 |
| 6 | `src/platform/win32/window.c` 的 WndProc 中新增 `case WM_COMMAND:` 分支，处理 1001（提交 RESTORE_FROM_TRAY）和 1002（DestroyWindow） | 菜单项点击有响应 | 编译通过 |
| 7 | 验证：启动程序，左键托盘图标，窗口恢复前台 | 左键行为正确 | 手动验证 |
| 8 | 验证：右键托盘图标，弹出菜单，测试"显示窗口"和"退出" | 右键行为正确 | 手动验证 |
