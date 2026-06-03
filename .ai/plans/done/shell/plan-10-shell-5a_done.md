# Shell-5a — AppBar 注册与贴边模型（后续）

## ① 核心问题

窗口只能浮动置顶或普通显示，没法"贴在桌面边缘"。用户想把便签靠在屏幕右边，最大化其他窗口时自动避让——做不到。

## ② 分析现状

### 阶段摘要

从"没有 AppBar 能力"推进到"可贴边占位（左/右/上/下），最大化窗口自动避让"。

### 承接关系

前置阶段：Shell-4_done（resident_mode 状态机、命令管线、state.ini 字段已就绪）

## ③ 方案设计

只有一条合理路径：新增 `src/platform/win32/appbar.h/c` 封装 SHAppBarMessage 调用，通过现有命令管线走 toggle 切换。不走 app 直接调——SHAppBarMessage 是系统 API，应封装在 platform 层，与 Shell-4 的 SetWindowPos 处理方式一致。

## ④ 拆解执行

### 整体目标

1. AppShellResidentMode 新增 `APP_SHELL_RESIDENT_MODE_EDGE_RESERVED`
2. 新增 appbar.h/c，封装 SHAppBarMessage（ABM_NEW / ABM_REMOVE / ABM_QUERYPOS / ABM_SETPOS）
3. 进入 edge_reserved 时：注册 AppBar + 设置贴边位置 + 保存 dock_edge/dock_thickness 到 state.ini
4. 退出 edge_reserved 时：注销 AppBar + 恢复窗口为普通模式
5. 从 floating_topmost 切到 edge_reserved 时：先取消 topmost，再注册 AppBar

### 阶段产出

1. 标题栏菜单可切换"贴边占位"
2. 贴边后最大化窗口不覆盖便签
3. 切回普通/浮动置顶时工作区恢复正常，无残留

## ⑤ 设定边界

### 本阶段范围

只做 AppBar 注册/注销的基本路径。不做异常恢复（Shell-5b）。

### 不包括

1. 不做分辨率变化后的重注册
2. 不做 Explorer 崩溃恢复
3. 不做多显示器

### 分层归属

- **platform/appbar** — SHAppBarMessage 封装（注册/注销/查询/设置位置）
- **platform/window** — 命令收束 switch 中调 appbar 接口
- **app/** — resident_mode 切换判断、触发 edge 持久化
- **ui/titlebar** — 菜单项提交命令
- **storage/** — dock_edge、dock_thickness 读写

### 文件落点

#### 预计新增文件

- `src/platform/win32/appbar.h`
- `src/platform/win32/appbar.c`

#### 预计修改文件

- `src/app/app.h` — AppShellResidentMode 新增 EDGE_RESERVED
- `src/app/app.c` — 切换命令处理
- `src/platform/win32/window.c` — 命令收束 switch 新增分支
- `src/storage/state_store.h` — 新增 dock_edge, dock_thickness
- `src/storage/state_store.c` — 读写新字段

#### 原则上不应修改

- `src/render/*`、`src/editor/*`、`src/core/*`

## ⑥ 落地方案

### 技术路线

**appbar.h：**

```c
#ifndef DESKNOTE_APPBAR_H
#define DESKNOTE_APPBAR_H

#include <windows.h>

/* AppBar 贴边方向 */
typedef enum {
    APP_DOCK_LEFT = 0,
    APP_DOCK_RIGHT,
    APP_DOCK_TOP,
    APP_DOCK_BOTTOM
} AppDockEdge;

/* 注册当前窗口为 AppBar，返回 0 成功 */
int AppBar_Register(HWND hwnd);

/* 注销 AppBar，返回 0 成功 */
int AppBar_Unregister(HWND hwnd);

/* 设置贴边位置，传入 edge 和 thickness */
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness);

/* 获取当前 AppBar 是否已注册 */
int AppBar_IsRegistered(HWND hwnd);

/* 重注册 AppBar（Shell-5b），内部自行判断是否需要恢复 */
int AppBar_ReRegister(HWND hwnd);

/* 从 state.ini 读取 dock 配置（Shell-5b） */
int AppBar_ReadDockConfig(AppDockEdge* out_edge, int* out_thickness);

#endif
```

**appbar.c 核心实现：**

```c
#define APPBAR_DATA (L"DeskNoteAppBar")

typedef struct {
    APPBARDATA data;
    int registered;
} AppBarState;

static AppBarState s_appbar = {0};

int AppBar_Register(HWND hwnd)
{
    if (s_appbar.registered)
        return 0;

    memset(&s_appbar.data, 0, sizeof(s_appbar.data));
    s_appbar.data.cbSize = sizeof(s_appbar.data);
    s_appbar.data.hWnd = hwnd;
    s_appbar.data.uCallbackMessage = WM_APP + 2; /* AppBar 通知消息 */

    if (!SHAppBarMessage(ABM_NEW, &s_appbar.data))
        return 1;

    s_appbar.registered = 1;
    return 0;
}

int AppBar_Unregister(HWND hwnd)
{
    if (!s_appbar.registered)
        return 0;

    SHAppBarMessage(ABM_REMOVE, &s_appbar.data);
    s_appbar.registered = 0;
    return 0;
}

int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);

    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;

    /* 根据贴边方向和厚度设置保留区域 */
    switch (edge)
    {
    case APP_DOCK_LEFT:
        s_appbar.data.rc.right = rc.left + thickness;
        break;
    case APP_DOCK_RIGHT:
        s_appbar.data.rc.left = rc.right - thickness;
        break;
    case APP_DOCK_TOP:
        s_appbar.data.rc.bottom = rc.top + thickness;
        break;
    case APP_DOCK_BOTTOM:
        s_appbar.data.rc.top = rc.bottom - thickness;
        break;
    }

    /* ABM_QUERYPOS 让系统调整建议位置 */
    SHAppBarMessage(ABM_QUERYPOS, &s_appbar.data);
    /* ABM_SETPOS 最终确认 */
    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    /* 移动窗口到保留区域 */
    MoveWindow(hwnd,
               s_appbar.data.rc.left,
               s_appbar.data.rc.top,
               s_appbar.data.rc.right - s_appbar.data.rc.left,
               s_appbar.data.rc.bottom - s_appbar.data.rc.top,
               TRUE);

    return 0;
}

int AppBar_IsRegistered(HWND hwnd)
{
    (void)hwnd;
    return s_appbar.registered;
}
```

**app.c 菜单命令处理中更新状态 + 持久化（cmd == 103 贴边占位菜单项）：**

在 `App_SubmitShellCommand` 的 `SHOW_MENU` 命令中，`cmd == 103` 分支。先更新 resident_mode、保存 dock 配置到 state.ini，再提交命令给 window.c 执行 AppBar 操作——app 层管状态和持久化，platform 层只执行系统调用。

```c
else if (cmd == 103) /* 贴边占位 */
{
    /* 切换 resident_mode */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
    {
        /* 当前已贴边 → 将要切换为 NONE */
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
    }
    else
    {
        /* 当前未贴边 → 将要切换为 EDGE_RESERVED */
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;

        /* 保存 dock 配置到 state.ini */
        StateData state;
        StateStore_Load(&state);
        state.dock_edge = APP_DOCK_RIGHT;
        state.dock_thickness = 240;
        StateStore_Save(&state);
    }

    /* 提交命令给 window.c，由它执行 AppBar_Register/Unregister */
    App_SubmitShellCommand(APP_SHELL_COMMAND_ENTER_EDGE_RESERVED);
}
```

**window.c 命令收束 switch 中新增分支——只调 AppBar API，不碰 app 状态：**

```c
case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:
    if (AppBar_IsRegistered(hwnd))
    {
        /* 已贴边 → 注销 AppBar */
        AppBar_Unregister(hwnd);
    }
    else
    {
        /* 未贴边 → 注册 AppBar 并设置位置 */
        /* dock_edge 和 thickness 已在 app 层的菜单命令中预存到 state.ini */
        if (AppBar_Register(hwnd) == 0)
        {
            AppBar_SetPosition(hwnd, APP_DOCK_RIGHT, 240);
        }
    }
    break;
```

注意：`ENTER_EDGE_RESERVED` 命令在 `App_GetShellCommandGroupInternal` 中已映射到 `SHELL_MODES` 组。菜单项点击后提交此命令即可。

### 主链路

```text
用户切换贴边占位
-> 标题栏菜单命令 cmd == 103
-> app.c 判断当前 resident_mode
-> 如果 EDGE_RESERVED → mode = NONE（退出贴边）
-> 如果非贴边 → mode = EDGE_RESERVED + 保存 dock 配置到 state.ini
-> App_SubmitShellCommand(ENTER_EDGE_RESERVED)
-> WM_TIMER 收束 → window.c switch 分支
-> 如果 AppBar_IsRegistered → AppBar_Unregister
-> 如果未注册 → AppBar_Register → AppBar_SetPosition
```

### 接口说明

```c
int AppBar_Register(HWND hwnd);
int AppBar_Unregister(HWND hwnd);
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness);
int AppBar_IsRegistered(HWND hwnd);
```

调用顺序：Register → SetPosition → ... → Unregister

## ⑦ 验收标准

1. 标题栏菜单出现"贴边占位"切换项
2. 切换后窗口贴到屏幕右侧，保留 240px 工作区
3. 最大化其他窗口时不覆盖便签区域
4. 退出贴边后工作区恢复正常
5. 从浮动置顶切到贴边时先取消 topmost
6. 编译通过，分层归属与文件落点一致

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/app/app.h` 的 `AppShellResidentMode` 枚举新增 `APP_SHELL_RESIDENT_MODE_EDGE_RESERVED`——扩展常驻模式枚举 | 枚举就绪 | 编译通过 |
| 2 | `src/platform/win32/appbar.h` 和 `appbar.c` 新增，实现 AppBar_Register/Unregister/SetPosition——SHAppBarMessage 封装在 platform 层，app 层不直接调 | appbar 模块就绪 | 编译通过 |
| 3 | `src/platform/win32/window.c` 的命令收束 switch 中新增 `ENTER_EDGE_RESERVED` 分支，只调 AppBar_Register/Unregister——window 层只做系统调用，不调 `App_SetResidentMode` | 贴边切换逻辑就绪 | 编译通过 |
| 4 | `src/storage/state_store.h` 新增 `dock_edge`、`dock_thickness` 字段；`state_store.c` 中增加读写——贴边配置持久化 | 持久化字段就绪 | 编译通过 |
| 5 | `src/app/app.c` 的标题栏菜单中新增"贴边占位"菜单项（cmd == 103），在分支中先更新 resident_mode、保存 dock 配置到 state.ini，再提交 `ENTER_EDGE_RESERVED` 命令——app 层管状态和持久化，platform 层只执行系统调用 | 菜单项出现，状态和持久化在 app 层完成 | 编译通过 |
| 6 | 验证：切换贴边，窗口贴右，最大化其他窗口不覆盖 | 贴边行为正确 | 手动验证 |
| 7 | 验证：退出贴边，工作区恢复 | 退出正确 | 手动验证 |
