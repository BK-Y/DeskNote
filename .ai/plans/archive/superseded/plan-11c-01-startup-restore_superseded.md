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

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [human] `state.ini` 位于 `%LOCALAPPDATA%\DeskNote\state.ini`

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 编译语法 | `gcc -fsyntax-only src/app/app.c` | 0 error |
| A-2 | StateStore_Save 残留检查 | `grep -c 'StateStore_Save' src/app/app.c` | ≤1 行（仅文档管理） |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 | 已知问题 |
|------|---------|---------|---------|
| M-1 | 1. 编辑 state.ini 设 `shell_resident_mode=2`<br>2. 启动应用 | 窗口贴右边缘，AppBar 注册 | — |
| M-2 | 1. 编辑 state.ini 设 `shell_resident_mode=1`<br>2. 设 `last_floating_left=100`、`last_floating_top=200`、`last_floating_width=240`、`last_floating_height=388`<br>3. 启动应用 | 窗口在屏幕 (100,200) 位置，置顶 | ⚠️ A-2 卡点：冷启动浮动坐标可能为 (0,0) |
| M-3 | 1. 编辑 state.ini 设 `shell_resident_mode=0`<br>2. 启动应用 | 普通窗口，无特殊置顶/贴边 | — |

### GATE 3 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈

> **已知问题**：M-2（冷启动浮动坐标 0,0）的根因见 `plan-11-test.md` → `当前阻塞` 分析。当前 GATE 3 因此阻塞。
