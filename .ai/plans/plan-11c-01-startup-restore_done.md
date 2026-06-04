# Plan-11c-01 — 启动恢复

## 核心问题

启动时 App_Init 和 App_TryRegisterAppBarFromState 直接调 StateStore_Load，不能从 Config 读数据。

## 前置

Config_Init 已调用，state.ini 已加载到内存表。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| A：替代 App_TryRegisterAppBarFromState | `StateData state; StateStore_Load(&state); if (state.shell_resident_mode != EDGE_RESERVED) return;` → `int mode = Config_Get(KEY_SHELL_RESIDENT_MODE, NONE); if (mode != EDGE_RESERVED) return;`；dock_edge/dock_thickness 同理 | 启动时 AppBar 注册路径改用 Config_Get 读取 mode/dock 参数 | state.ini `shell_resident_mode=2` 重启 → AppBar 注册，窗口贴边 |
| B：替代启动时浮动恢复 | `StateData state; StateStore_Load(&state); if (state.shell_resident_mode == FLOATING_TOPMOST) { SetWindowPos(hwnd, HWND_TOPMOST, state.last_floating_*, SWP_NOZORDER); }` → `int mode = Config_Get(KEY_SHELL_RESIDENT_MODE, NONE); if (mode == FLOATING_TOPMOST) SetWindowPos(hwnd, HWND_TOPMOST, Config_Get(KEY_FLOATING_LEFT,0), Config_Get(KEY_FLOATING_TOP,0), Config_Get(KEY_FLOATING_WIDTH,240), Config_Get(KEY_FLOATING_HEIGHT,388), SWP_NOZORDER);` | 启动时浮动恢复路径改用 Config_Get | `shell_resident_mode=1` + floating 坐标有效 → 重启 → 浮动置顶 |
| C：替代启动时 presence_state 读取 | `StateData state; StateStore_Load(&state); if (state.presence_state == 1) ShowWindow(hwnd, SW_HIDE);` → `int presence = Config_Get(KEY_PRESENCE_STATE, 0); if (presence == SHELL_PRESENCE_HIDDEN_TO_TRAY) ShowWindow(hwnd, SW_HIDE);` | 启动时隐藏恢复路径改用 Config_Get | state.ini `presence_state=1` 重启 → 窗口不显示，仅托盘图标 |
| D：titlebar_height / frame_visual_thickness | `App_ApplyShellStateData` 中从 StateData 读这两个字段 → `g_app.shell.titlebar_height = Config_Get("titlebar_height", CONFIG_DEFAULT_TITLEBAR_HEIGHT); g_app.shell.frame_visual_thickness = Config_Get("frame_visual_thickness", CONFIG_DEFAULT_FRAME_VISUAL_THICKNESS);` | 启动时窗口度量字段从 Config 读取 | 配置值变化后重启，标题栏高度/边框厚度生效 |

## 主链路

```
App_Init → Config_Get("shell_resident_mode") → EDGE_RESERVED? → AppBar_Register + AppBar_SetPosition
         → Config_Get("presence_state")      → HIDDEN? → ShowWindow(SW_HIDE)
         → Config_Get("titlebar_height")     → g_app.shell.titlebar_height
```

## 验收标准

- [ ] StateStore_Load 在启动恢复路径中被 Config_Get 替换
- [ ] state.ini `shell_resident_mode=2` 重启后 AppBar 注册

## 分层归属

本步骤属于 app 层，将 app.c 中的 StateStore_Load 改为 Config_Get，不跨层。
11c-01 启动恢复结束后，使用 `git add src/app/app.c` 做一次增量提交。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
