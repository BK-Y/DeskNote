# Shell-5 — 便签壳层：贴边占位与状态提示（后续）

## ① 核心问题

浮动置顶只是让窗口浮在最前，没有"贴在桌面边缘占位"的能力。用户想把便签固定在屏幕右侧，最大化其他窗口时不覆盖它——当前做不到。

另外，切换常驻模式后（浮动置顶/贴边占位），用户从托盘图标上看不到当前是什么模式，需要点开菜单才能确认。

## ② 分析现状

### 承接关系

前置阶段（已完成）：

- Shell-3b — presence 状态管理已就绪
- Shell-3c_2 — 状态持久化已就绪（presence_state 读写）
- Shell-4_done — floating topmost 状态机已就绪；bounds 持久化已就绪

本阶段消费 Shell-4 的产物：

- `AppShellResidentMode` 当前有 `NONE`、`FLOATING_TOPMOST`
- `App_SubmitShellCommand` 命令管线可用
- `window.c` 的命令收束 switch 可扩展
- `last_floating_*` 字段已在 state.ini 中

### 阶段摘要

从"只有浮动置顶"推进到"支持贴边占位（AppBar），且所有常驻模式的状态可通过托盘图标一眼识别"。

## ③ 方案设计

**方案 A：Shell-5 拆为三个子阶段（AppBar 核心 → 异常恢复 → 状态提示）[selected]**

- 5a：AppBar 注册/注销、贴边方向、dock thickness 建模
- 5b：分辨率变化、Explorer 重启后的恢复
- 5c：常驻模式状态提示（托盘 hover + 菜单状态标记）

理由：
- 5a 做完可验证"窗口能贴边且其他窗口会避让"
- 5b 是纯异常处理，不改变 5a 的行为
- 5c 需要 resident_mode 全部稳定后再做，一次覆盖所有模式
- 三个子阶段互不依赖对方的代码改动

**方案 B：合并为一个阶段全部实现**

- 缺点：同时面对 AppBar API + 异常恢复 + 状态提示，改动量过大，难以定位问题

**方案 A 选中。**

## ④ 拆解执行

### 子阶段列表

| 子阶段 | 状态 | 主题 | 主要改动文件 | 依赖 |
|--------|------|------|-------------|------|
| Shell-5a | **已完成** | AppBar 注册与贴边模型 | appbar.h/c, app.h/c, state_store.h/c | Shell-4 |
| Shell-5b | 后续 | 异常恢复与重同步 | appbar.c, window.c, app.c | 5a |
| Shell-5c | 后续 | 常驻模式状态提示 | app.c, window.c | 5b |

### 依赖关系

```text
Shell-4
  ↓
5a (AppBar 核心：注册/注销/贴边)
  ↓
5b (异常恢复：分辨率/Explorer/重协商)
  ↓
5c (状态提示：托盘 hover + 菜单标记)
```

### 整体目标

1. 支持 AppBar 贴边占位（左/右/上/下），最大化窗口自动避让
2. 分辨率变化和 Explorer 重启后自动恢复
3. 托盘图标 hover 提示当前常驻模式
4. 标题栏菜单中标记当前激活的常驻模式

### 阶段产出

1. 窗口可贴边，最大化不侵入
2. 切回浮动置顶或普通模式时无残留
3. 托盘图标 hover 显示"DeskNote — 浮动置顶"或"DeskNote — 贴边占位"
4. 菜单中当前模式前显示 ✓

## ⑤ 设定边界

### 本阶段范围

AppBar 贴边 + 异常恢复 + 状态提示。

### 不包括

1. 不做多便签并排停靠
2. 不做多显示器复杂工作区分配
3. 不做自动吸附推荐

### 分层归属

| 层 | 职责 |
|------|------|
| platform/appbar | SHAppBarMessage 封装（注册/注销/更新/查询） |
| platform/window | 命令收束 switch 中调 appbar 接口；WM_DISPLAYCHANGE 转发 |
| app/ | resident_mode 扩展、切换状态机、触发 bounds/edge 持久化 |
| ui/titlebar | 菜单项提交命令；菜单中标记当前模式 |
| storage/ | dock_edge、dock_thickness 读写 |

### 文件落点

#### 预计新增文件

- `src/platform/win32/appbar.h`
- `src/platform/win32/appbar.c`

#### 预计修改文件

- `src/app/app.h` — AppShellResidentMode 扩展
- `src/app/app.c` — 切换逻辑 + 状态提示触发
- `src/platform/win32/window.c` — 命令收束 + 系统事件转发
- `src/storage/state_store.h` — 新增 dock_edge, dock_thickness
- `src/storage/state_store.c` — 读写新字段

#### 原则上不应修改

- `src/render/*`、`src/editor/*`、`src/core/*`

## ⑥ 落地方案

本阶段不涉及——技术路线由各子阶段的详细计划展开。

## ⑦ 验收标准

1. 贴边后最大化窗口不侵入保留区
2. 切回普通/浮动置顶时工作区恢复正常
3. 分辨率变化后贴边保留区自动重建
4. Explorer 重启后 AppBar 自动重注册
5. 托盘 hover 提示当前模式
6. 菜单中当前模式显示 ✓ 标记

## ⑧ 推进步骤

本文件为纲领文件，推进步骤由各子阶段的详细计划提供。
