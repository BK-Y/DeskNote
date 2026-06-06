# Shell-2 — 便签壳层：窗口交互语义（后续）

## 阶段摘要

把系统从"已有自绘标题栏和 Button 组件"推进到"标题栏菜单按钮可弹出菜单、标题栏可拖拽移动、窗口可缩放，桌面交互语义完整"的状态。

## 承接关系

前置阶段（已完成）：

- `Shell-1d_done` — 标题栏组件与边框视觉基线（含 close + minimize 按钮视觉）
- `Shell-1e_done` — Button 组件 + 白边修复

本阶段直接消费 Shell-1d/1e 的输出，并将 close/minimize 按钮替换为单一 menu 按钮：

- `TitlebarLayout`（Shell-2a 中将 `close_button` + `minimize_button` 替换为 `menu_button`）
- `Button_HitTest` / `Button_UpdateState` / `Button_Draw`
- frameless 窗体 + 自绘边框基线

## 工作核心

1. 标题栏按钮从"关闭+最小化"改为"单个菜单按钮"，点击弹出上下文菜单
2. 标题栏非按钮区可拖拽移动窗口
3. 窗口四边/四角可缩放
4. 按钮点击与拖拽不串扰

## 整体目标

1. 标题栏右上角有一个菜单按钮，点击弹出菜单（含"关闭"和"关于"）
2. "关于"对话框显示 DeskNote 版本和 md4c 版权信息
3. 标题栏非按钮区可拖拽移动窗口
4. 窗口四边/四角可缩放
5. 按钮点击不误触拖拽，拖拽不吞按钮点击

## 约束要求

1. **按钮点击不触拖拽** — `Titlebar_HitTest` 先检查菜单按钮区，再检查拖拽区
2. **命令走 `AppShellCommand`** — 菜单按钮点击通过 `App_SubmitShellCommand` 分发，`ui/titlebar` 不直接操作窗口
3. **关于对话框使用 Win32 原生 MessageBox** — 不引入新的 UI 组件，md4c 版权信息硬编码在 `App_OnShellCommand` 中
4. **不新增 render 原语**
5. **不修改 editor_view / storage / editor / core**
6. **为后续可配置按钮预留** — `titlebar_command_groups` 字段保留，当前只显示菜单按钮；未来设置面板可启用其他按钮

## 子阶段列表

| 子阶段 | 当前状态 | 主题 |
|--------|---------|------|
| Shell-2a | 后续（当前焦点） | 菜单按钮 + 弹出菜单 + 标题栏拖拽 |
| Shell-2b | 后续 | 四边/四角缩放 |

### 依赖关系

```text
Shell-1e_done
  ↓
Shell-2a (菜单按钮 + 拖拽)
  ↓
Shell-2b (缩放)
  ↓
Shell-3a 及后续
```

### Shell-2a / 2b 边界

| 能力 | 归属 | 说明 |
|------|------|------|
| TitlebarLayout 移除 close_button + minimize_button | Shell-2a | 替换为单个 `menu_button` |
| 菜单按钮点击 → 弹出菜单 | Shell-2a | `TrackPopupMenu` + `APP_SHELL_COMMAND_SHOW_MENU` |
| 菜单项"关闭" → 退出 | Shell-2a | `PostQuitMessage` |
| 菜单项"关于" → 显示版权 | Shell-2a | `MessageBox` 显示 DeskNote 版本 + md4c MIT 声明 |
| 标题栏非按钮区 → 拖拽 | Shell-2a | `Platform_BeginWindowDrag(hwnd)` |
| 替换 Shell-1b 的"整个顶区 HTCAPTION" | Shell-2a | 限制为仅非按钮区 |
| 窗口边缘缩放 | Shell-2b | `WM_NCHITTEST` → `HTLEFT`/`HTRIGHT`/`HTTOP`/`HTBOTTOM` 等 |
| 标题栏按钮可配置化 | 不属于 Shell-2 | 后续设置面板 + Shell-1a 的 `titlebar_command_groups` |

## 读取规则

进入 Shell-2 时：

1. 先读 `plan.md` 确认当前焦点
2. 再读 `phase-shell-2.md`（本文件）了解整体目标和约束
3. 最后只读当前焦点的子阶段
4. 核对边界时读 `docs/design/architecture.md`

## 计划的最后一步

1. `git add` 当前阶段修改过的 shell 计划文件
2. `git commit` 使用规范提交说明
3. `git push` 提交到远端仓库
