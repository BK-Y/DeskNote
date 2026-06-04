# Plan-11c-02 — 菜单进入/退出贴边

## 核心问题

cmd103 handler 直接调 StateStore_Load/Save 读写 dock_edge/dock_thickness/shell_resident_mode，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| A：进入 EDGE_RESERVED | `StateStore_Load → 改 dock_edge / dock_thickness / shell_resident_mode → StateStore_Save → AppBar_Register + SetPosition` → `Config_Set(KEY_DOCK_EDGE/KEY_DOCK_THICKNESS/KEY_SHELL_RESIDENT_MODE); if (!AppBar_IsRegistered) { AppBar_Register; AppBar_SetPosition; }` | cmd103 进入贴边路径改用 Config_Set | 菜单→贴边占位 → 窗口贴边，state.ini 中 mode=2 + dock 值更新 |
| B：退出 EDGE_RESERVED（回到 NONE） | `g_app.shell.resident_mode = NONE + AppBar_Unregister + MoveToCenter` → `Config_Set(KEY_SHELL_RESIDENT_MODE, NONE); if (AppBar_IsRegistered) AppBar_Unregister(hwnd); { RECT rc; GetWindowRect(hwnd, &rc); int w = rc.right-rc.left; int h = rc.bottom-rc.top; int x = rc.left; int y = rc.top; SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0); int cw = rc.right-rc.left; int ch = rc.bottom-rc.top; if (x + w >= rc.right - 10) x = (cw - w) / 2; if (y + h >= rc.bottom - 10) y = (ch - h) / 2; MoveWindow(hwnd, x, y, w, h, TRUE); }` | cmd103 退出贴边路径改用 Config_Set，窗口移回中央 | 贴边→再点"贴边占位" → 窗口移回中央，state.ini mode=0 |

## 主链路

```
菜单→贴边 → cmd103 → Config_Set(KEY_DOCK_EDGE) → Config_Set(KEY_DOCK_THICKNESS) → Config_Set(KEY_SHELL_RESIDENT_MODE) → AppBar_Register + SetPosition
菜单→退出贴边 → cmd103 → Config_Set(KEY_SHELL_RESIDENT_MODE, NONE) → AppBar_Unregister → MoveToCenter
```

## 验收标准

- [ ] cmd103 进入/退出贴边路径中 StateStore_Load/Save 被 Config_Set 替换
- [ ] 菜单→贴边→退出贴边，state.ini 中 mode/dock 值正确

## 分层归属

本步骤属于 app 层，将 cmd103 handler 中的 StateStore 操作改为 Config_Set，属于同层内部替换。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
