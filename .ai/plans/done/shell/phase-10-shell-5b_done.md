# Shell-5b — 异常恢复与重同步

## ① 核心问题

AppBar 贴边在正常路径下工作正常，但分辨率变化、任务栏调整、Explorer 崩溃重启后，AppBar 注册丢失且不会自动恢复——窗口可能错位，工作区保留变成脏状态。

## ② 前置条件（已就绪）

- `appbar.h/c`：Register / Unregister / SetPosition / IsRegistered / ReRegister / ReadDockConfig 均可正常调用
- `app.c`：菜单 cmd==103 贴边切换逻辑已就绪（同步执行 AppBar_Register/Unregister）
- `AppBar_Register` 已设置 `uCallbackMessage = WM_APP + 2`，但 WndProc 无处理分支

**注意事项（相对原计划的变更）：**
- `ENTER_EDGE_RESERVED` 已不再走异步命令队列，cmd 103 同步执行 AppBar 注册/注销。`window.c` 中 `WM_TIMER` 下的 `case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED` 是死代码，实施前需删除。

## ③ 方案设计

监听系统事件触发重注册，被动恢复：
- `WM_DISPLAYCHANGE`（分辨率变化）
- `WM_SETTINGCHANGE`（任务栏调整，wParam==SPI_SETWORKAREA）
- `WM_APP+2`（AppBar 回调：ABN_POSCHANGED / ABN_FULLSCREENAPP）
- `RegisterWindowMessageW("TaskbarCreated")`（Explorer 重启）

## ④ 实施前准备工作

### 4.1 删除死代码

`src/platform/win32/window.c` 中 `WM_TIMER` handler 下的 `case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:` 整个分支已无人使用，删除。

### 4.2 新增公开接口

`src/platform/win32/appbar.h` 新增：

```c
/* 获取当前 AppBar 贴边方向（用于 WM_APP+2 回调中重协商） */
AppDockEdge AppBar_GetEdge(void);
/* 获取当前 AppBar 厚度 */
int AppBar_GetThickness(void);
```

`src/platform/win32/appbar.c` 实现：

```c
AppDockEdge AppBar_GetEdge(void)
{
    return (AppDockEdge)s_appbar.data.uEdge;
}
int AppBar_GetThickness(void)
{
    return s_appbar.data.rc.right - s_appbar.data.rc.left;
}
```

## ⑤ 具体实现

### 5.1 TaskbarCreated 消息注册

在 `Window_Run` 中（或窗口初始化时）注册一次：

```c
UINT g_msgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
```

在 WndProc 中处理：

```c
if (msg == g_msgTaskbarCreated)
{
    AppBar_ReRegister(hwnd);
    return 0;
}
```

### 5.2 WM_DISPLAYCHANGE

```c
case WM_DISPLAYCHANGE:
    AppBar_ReRegister(hwnd);
    return 0;
```

### 5.3 WM_SETTINGCHANGE

```c
case WM_SETTINGCHANGE:
    if (wParam == SPI_SETWORKAREA && AppBar_IsRegistered(hwnd))
    {
        AppDockEdge edge;
        int thickness;
        if (AppBar_ReadDockConfig(&edge, &thickness) == 0)
            AppBar_SetPosition(hwnd, edge, thickness);
    }
    return 0;
```

### 5.4 WM_APP+2（AppBar 回调）

使用新增的公开接口，不直接访问 `s_appbar`：

```c
case WM_APP + 2:
    switch (wParam)
    {
    case ABN_POSCHANGED:
        AppBar_SetPosition(hwnd, AppBar_GetEdge(), AppBar_GetThickness());
        break;
    case ABN_FULLSCREENAPP:
        AppBar_ReRegister(hwnd);
        break;
    }
    return 0;
```

### 5.5 WM_DESTROY 退出清理

```c
case WM_DESTROY:
    KillTimer(hwnd, WINDOW_AUTOSAVE_TIMER_ID);
    AppBar_Unregister(hwnd);          /* 新增 */
    App_DestroyTrayIcon(hwnd);
    App_Shutdown();
    Platform_Nonclient_Shutdown(hwnd);
    PostQuitMessage(0);
    return 0;
```

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/platform/win32/appbar.h` | 新增 `AppBar_GetEdge()` / `AppBar_GetThickness()` |
| `src/platform/win32/appbar.c` | 实现上述两个函数 |
| `src/platform/win32/window.c` | 删除死代码分支；新增 4 个 handler + TaskbarCreated 注册 + WM_DESTROY 中加 Unregister |

不应修改：`src/app/*`、`src/storage/*`、`src/render/*`、`src/editor/*`、`src/core/*`。

## ⑦ 推进步骤

| 步骤 | 操作 | 验证 |
|------|------|------|
| 1 | 删除 `case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED` 死代码 | 编译通过 |
| 2 | 新增 `AppBar_GetEdge` / `AppBar_GetThickness` 公开接口 | 编译通过 |
| 3 | 注册 `TaskbarCreated` 消息 + WndProc handler | 编译通过 |
| 4 | 新增 `WM_DISPLAYCHANGE` handler | 编译通过 |
| 5 | 新增 `WM_SETTINGCHANGE` handler | 编译通过 |
| 6 | 新增 `WM_APP+2` handler | 编译通过 |
| 7 | `WM_DESTROY` 中插入 `AppBar_Unregister` | 编译通过 |
| 8 | 手动验证：贴边后切换分辨率、重启 Explorer | 保留区自动重建 |

## ⑧ 分层核实

- 所有新增 handler 仅调 `appbar.h` 公开接口（`AppBar_ReRegister` / `AppBar_SetPosition` / `AppBar_ReadDockConfig` / `AppBar_GetEdge` / `AppBar_GetThickness`），不调 app 层函数
- `WM_APP+2` handler 使用 `AppBar_GetEdge()` / `AppBar_GetThickness()`，不直接访问 `s_appbar`
- `WM_DESTROY` 中 `AppBar_Unregister` 位于 `App_Shutdown()` 之前，仅调 platform 层函数
- `app.c` / `app.h` 零改动
