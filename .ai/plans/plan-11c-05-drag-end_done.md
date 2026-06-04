# Plan-11c-05 — 拖拽结束

## 核心问题

App_OnEndDrag 直接调 StateStore_Save 保存模式和窗口几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| A：拖拽→浮动置顶（release_on_drag_mode=1） | `StateData state; StateStore_Load(&state); state.last_floating_* = rc.*; state.shell_resident_mode = FLOATING_TOPMOST; StateStore_Save(&state); AppBar_Unregister; g_app.shell.resident_mode = FLOATING_TOPMOST; SetWindowPos(HWND_TOPMOST)` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rc.*); Config_Set(KEY_SHELL_RESIDENT_MODE, FLOATING_TOPMOST); g_app.shell.resident_mode = FLOATING_TOPMOST; AppBar_Unregister; SetWindowPos(HWND_TOPMOST)` | 拖拽→浮动路径改用 Config_Set 持久化 mode 和几何 | 贴边→拖离边缘 → 变为浮动置顶窗口，state.ini 中 mode=1 + floating 坐标更新 |
| B：拖拽→普通窗口（release_on_drag_mode=2） | `state.shell_resident_mode = NONE; StateStore_Save(&state); AppBar_Unregister; g_app.shell.resident_mode = NONE;` → `Config_Set(KEY_SHELL_RESIDENT_MODE, NONE); g_app.shell.resident_mode = NONE; AppBar_Unregister;` | 拖拽→普通窗口路径改用 Config_Set 持久化 mode | 贴边→拖离（mode=2） → 变为普通窗口，不置顶 |

## 主链路

```
拖拽结束 → WM_EXITSIZEMOVE → App_OnEndDrag → GetWindowRect → Config_Set(last_floating_*) + Config_Set(mode) → AppBar_Unregister → SetWindowPos
```

## 验收标准

- [ ] App_OnEndDrag 中的 StateStore 调用被 Config_Set 替换
- [ ] state.ini 中 mode + 几何数据更新正确

## 分层归属

本步骤属于 app 层，将 App_OnEndDrag 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
