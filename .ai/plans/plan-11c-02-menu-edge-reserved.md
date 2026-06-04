# Plan-11c-02 — 菜单进入/退出贴边

## 目标
将 cmd103 分支中的 StateStore 读/写替换为 Config_Set。

## 前置
Config_Init 已调用。

## 步骤 A：进入 EDGE_RESERVED

原代码（app.c L1430-1480）：StateStore_Load → 改 dock_edge / dock_thickness / shell_resident_mode → StateStore_Save → AppBar_Register + SetPosition

改为：

```c
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;
Config_Set(KEY_DOCK_EDGE, APP_DOCK_RIGHT);
Config_Set(KEY_DOCK_THICKNESS, 240);
Config_Set(KEY_SHELL_RESIDENT_MODE, APP_SHELL_RESIDENT_MODE_EDGE_RESERVED);

HWND hwnd = g_app.hwnd;
if (!AppBar_IsRegistered(hwnd)) {
    AppBar_Register(hwnd);
    AppBar_SetPosition(hwnd, APP_DOCK_RIGHT, 240);
}
```

验证：运行时菜单→贴边占位 → 窗口贴边，state.ini 中 mode=2 + dock 值更新。

## 步骤 B：退出 EDGE_RESERVED（回到 NONE）

原代码：`g_app.shell.resident_mode = NONE` + AppBar_Unregister + MoveToCenter

改为：

```c
g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
Config_Set(KEY_SHELL_RESIDENT_MODE, APP_SHELL_RESIDENT_MODE_NONE);

if (AppBar_IsRegistered(hwnd))
    AppBar_Unregister(hwnd);
// MoveToCenter 照旧
```

验证：贴边→再点"贴边占位" → 窗口移回中央，state.ini mode=0。

## 分层归属

本步骤属于 app 层，将 cmd103 handler 中的 StateStore 操作改为 Config_Set，属于同层内部替换。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
