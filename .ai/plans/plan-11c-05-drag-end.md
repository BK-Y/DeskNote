# Plan-11c-05 — 拖拽结束

## 目标
将 App_OnEndDrag 中的 StateStore_Save 替换为 Config_Set。

## 前置
Config_Init 已调用。

## 步骤 A：拖拽→浮动置顶（release_on_drag_mode=1）

原代码（app.c L1720-1734）：

```c
StateData state; StateStore_Load(&state);
state.last_floating_left   = rc.left;
state.last_floating_top    = rc.top;
state.last_floating_width  = rc.right - rc.left;
state.last_floating_height = rc.bottom - rc.top;
state.shell_resident_mode  = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
StateStore_Save(&state);
AppBar_Unregister(hwnd);
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
SetWindowPos(hwnd, HWND_TOPMOST, ...);
```

改为：

```c
Config_Set(KEY_FLOATING_LEFT, rc.left);
Config_Set(KEY_FLOATING_TOP, rc.top);
Config_Set(KEY_FLOATING_WIDTH, rc.right - rc.left);
Config_Set(KEY_FLOATING_HEIGHT, rc.bottom - rc.top);
Config_Set(KEY_SHELL_RESIDENT_MODE, APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST);
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
AppBar_Unregister(hwnd);
SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top,
             rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
```

验证：贴边→拖离边缘 → 变为浮动置顶窗口，state.ini 中 mode=1 + floating 坐标更新。

## 步骤 B：拖拽→普通窗口（release_on_drag_mode=2）

原代码：

```c
state.shell_resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
StateStore_Save(&state);
AppBar_Unregister(hwnd);
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
```

改为：

```c
Config_Set(KEY_SHELL_RESIDENT_MODE, APP_SHELL_RESIDENT_MODE_NONE);
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
AppBar_Unregister(hwnd);
```

验证：贴边→拖离（mode=2） → 变为普通窗口，不置顶。

## 分层归属

本步骤属于 app 层，将 App_OnEndDrag 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
