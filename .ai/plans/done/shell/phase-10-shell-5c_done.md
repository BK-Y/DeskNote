# Shell-5c — 常驻模式状态提示

## ① 核心问题

切换浮动置顶或贴边占位后，从托盘图标上看不到当前是什么模式。用户需要点开菜单才能确认"现在是浮动置顶还是贴边"。托盘图标 hover 提示始终是"DeskNote"，没有反映实际状态。

## ② 前置条件

- `resident_mode` 状态已稳定（`App_GetResidentMode()` 可用）
- 菜单命令处理分支已就绪（cmd 102/103 同步处理）
- cmd 103 已改为同步执行 AppBar 注册/注销，不再经过异步命令队列

## ③ 方案设计

### 3.1 托盘 hover 提示

在 `resident_mode` 切换时更新托盘图标 hover 提示文本。直接同步调用 `Shell_NotifyIconW(NIM_MODIFY)`，不经过异步命令队列。

### 3.2 菜单 ✓ 标记

在渲染菜单时检查 `g_app.shell.resident_mode`，对当前模式所在菜单项添加 `MF_CHECKED`。

## ④ 具体实现

### 4.1 托盘提示更新函数

在 `src/platform/win32/window.h` 或 `window.c` 中新增（或已有 `App_InitTrayIcon` 附近）：

```c
void App_UpdateTrayTip(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;

    AppShellResidentMode mode = App_GetResidentMode();
    const wchar_t* tip = L"DeskNote";
    if (mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        tip = L"DeskNote — 浮动置顶";
    else if (mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
        tip = L"DeskNote — 贴边占位";

    wcsncpy(nid.szTip, tip, 128);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}
```

### 4.2 在切换后调用

`resident_mode` 变化的每一处都要更新托盘提示：

| 变化来源 | 位置 | 说明 |
|---------|------|------|
| cmd 103 进入/退出贴边 | `app.c` 同步代码块末尾 | `resident_mode` 已同步设置 |
| cmd 102 浮动置顶 | `window.c` `WM_TIMER` 中 `TOGGLE_FLOATING_TOPMOST` 分支末尾 | `resident_mode` 在异步 handler 中才设置 |
| 托盘恢复时重置为 NONE | `app.c` `App_RestoreFromTray` 中，清零后立即调用 | `AppBar_IsRegistered` 为 false 时重置 |
| 拖动释放 AppBar | `app.c` `App_OnEndDrag` 函数末尾 | 模式切换到 TOPMOST 或 NONE 后 |

不需要新增枚举、不需要走异步命令队列。

### 4.3 菜单 ✓ 标记

在 `SHOW_MENU` 的 `AppendMenuW` 调用处：

```c
AppendMenuW(hMenu, MF_STRING |
    (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST ? MF_CHECKED : 0),
    102, L"浮动置顶");

AppendMenuW(hMenu, MF_STRING |
    (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED ? MF_CHECKED : 0),
    103, L"贴边占位");
```

### 4.4 启动时设置初始提示

在 `App_InitTrayIcon` 创建托盘图标后，调用一次 `App_UpdateTrayTip(hwnd)` 设置初始提示文本。

## ⑤ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/app/app.c` | 菜单项加 `MF_CHECKED`；cmd 103、`App_RestoreFromTray`、`App_OnEndDrag` 中各加 `App_UpdateTrayTip` 调用；`App_InitTrayIcon` 末尾调用一次 |
| `src/app/app.h` | 声明 `App_UpdateTrayTip` |
| `src/platform/win32/window.c` | `WM_TIMER` 中 `TOGGLE_FLOATING_TOPMOST` 分支末尾调用 `App_UpdateTrayTip` |

不应修改：`src/storage/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/platform/win32/appbar.*`。

## ⑥ 推进步骤

| 步骤 | 操作 | 验证 |
|------|------|------|
| 1 | 实现 `App_UpdateTrayTip` 函数 | 编译通过 |
| 2 | 菜单项添加 `MF_CHECKED` | 编译通过，菜单可见 ✓ |
| 3 | cmd 102/103 切换后调 `App_UpdateTrayTip` | 编译通过 |
| 4 | 启动初始化时调一次 `App_UpdateTrayTip` | 编译通过 |
| 5 | 验证：切换模式后 hover 提示变化 | 手动验证 |
| 6 | 验证：菜单中当前模式前有 ✓ | 手动验证 |

## ⑦ 分层核实

- `App_UpdateTrayTip` 从中性位置（app.c 或 window.c）调 `Shell_NotifyIconW(NIM_MODIFY)`，属于 platform 层接口
- 菜单 `MF_CHECKED` 仅在菜单渲染时读取 `g_app.shell.resident_mode`，无反向调用
- 不修改 `appbar.*`、`storage/*`、`render/*`、`editor/*`、`core/*`
