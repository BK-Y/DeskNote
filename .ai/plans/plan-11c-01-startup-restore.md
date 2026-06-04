# Plan-11c-01 — 启动恢复

## 目标
将启动时模式恢复的 StateStore 读替换为 Config_Get。

## 前置
Config_Init 已调用，state.ini 已加载到内存表。

## 步骤 A：替代 App_TryRegisterAppBarFromState

原代码（app.c L770-800）：

```c
StateData state; StateStore_Load(&state);
if (state.shell_resident_mode != EDGE_RESERVED) return;
AppDockEdge edge = (AppDockEdge)ClampDockEdge(state.dock_edge);
int thickness = ClampDockThickness(state.dock_thickness);
AppBar_Register(hwnd); AppBar_SetPosition(hwnd, edge, thickness);
g_app.shell.resident_mode = EDGE_RESERVED;
```

改为：

```c
int mode = Config_Get(KEY_SHELL_RESIDENT_MODE, NONE);
if (mode != EDGE_RESERVED) return;
int edge = Config_Get(KEY_DOCK_EDGE, APP_DOCK_RIGHT);
int thick = Config_Get(KEY_DOCK_THICKNESS, 240);
AppBar_Register(hwnd);
AppBar_SetPosition(hwnd, (AppDockEdge)edge, thick);
g_app.shell.resident_mode = EDGE_RESERVED;
```

验证：state.ini `shell_resident_mode=2` 重启 → AppBar 注册，窗口贴边。

## 步骤 B：替代启动时浮动恢复

原代码（app.c L880-890）：

```c
StateData state; StateStore_Load(&state);
if (state.shell_resident_mode == FLOATING_TOPMOST) {
    SetWindowPos(hwnd, HWND_TOPMOST,
        state.last_floating_left, state.last_floating_top,
        state.last_floating_width, state.last_floating_height, SWP_NOZORDER);
}
```

改为：

```c
int mode = Config_Get(KEY_SHELL_RESIDENT_MODE, NONE);
if (mode == FLOATING_TOPMOST) {
    int x = Config_Get(KEY_FLOATING_LEFT, 0);
    int y = Config_Get(KEY_FLOATING_TOP, 0);
    int w = Config_Get(KEY_FLOATING_WIDTH, 240);
    int h = Config_Get(KEY_FLOATING_HEIGHT, 388);
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOZORDER);
}
```

验证：`shell_resident_mode=1` + floating 坐标有效 → 重启 → 浮动置顶。

## 步骤 C：替代启动时 presence_state 读取

原代码（app.c L868-875）：

```c
StateData state; StateStore_Load(&state);
if (state.presence_state == 1) /* hidden_to_tray */
    ShowWindow(hwnd, SW_HIDE);
```

改为：

```c
int presence = Config_Get(KEY_PRESENCE_STATE, 0);
if (presence == SHELL_PRESENCE_HIDDEN_TO_TRAY)
    ShowWindow(hwnd, SW_HIDE);
```

验证：state.ini `presence_state=1` 重启 → 窗口不显示，仅托盘图标。

## 步骤 D：titlebar_height / frame_visual_thickness 读路径

当前 `App_ApplyShellStateData` 从 StateData 读这两个字段，改为 Config_Get：

```c
g_app.shell.titlebar_height = Config_Get("titlebar_height", CONFIG_DEFAULT_TITLEBAR_HEIGHT);
g_app.shell.frame_visual_thickness = Config_Get("frame_visual_thickness", CONFIG_DEFAULT_FRAME_VISUAL_THICKNESS);
```

## 分层归属

本步骤属于 app 层，将 app.c 中的 StateStore_Load 改为 Config_Get，不跨层。
11c-01 启动恢复结束后，使用 `git add src/app/app.c` 做一次增量提交。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
