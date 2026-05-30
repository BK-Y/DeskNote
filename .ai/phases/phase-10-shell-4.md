# Shell-4 — 便签壳层：浮动置顶常驻模式（后续）

## ① 核心问题

便签窗口只能普通显示。用户切到其他应用时便签就被盖住了，想要"一直浮在最前面"但做不到。

## ② 分析现状

### 承接关系

前置阶段（已完成）：

- Shell-3b — presence 状态管理已就绪（App_HideToTray / App_RestoreFromTray）
- Shell-3c_1 — 关闭策略已就绪
- Shell-3c_2 — 状态持久化已就绪（presence_state 读写）

本阶段消费 Shell-3 的产物：

- `g_app.shell.resident_mode` — 当前常驻模式（仅 NONE）
- `shell_resident_mode` — state.ini 中已有此字段
- `App_RestoreFromTray` — 可从其中读取 resident_mode 做恢复

### 阶段摘要

从"没有置顶能力"推进到"标题栏菜单可切换浮动置顶，重启后恢复"。

## ③ 方案设计

只有一条合理路径：titlebar 菜单响应点击后直接调 SetWindowPos 切换 topmost，同时更新 g_app.shell.resident_mode。App_RestoreFromTray 和 App_Init 中读取该 mode 做恢复。

不走命令管线的原因是：SetWindowPos 切换是单行系统调用，不需要 app 层做前置条件判断或多步协调，直接执行最简洁。

## ④ 拆解执行

### 子阶段列表

| 子阶段 | 状态 | 主题 | 主要改动文件 | 依赖 |
|--------|------|------|-------------|------|
| Shell-4a_1 | **已完成** | floating topmost 状态机 | app.h, app.c, titlebar.c, window.c | Shell-3c_2 |
| Shell-4a_2 | 后续 | floating topmost 持久化 | state_store.h/c, app.c | 4a_1 |

### 依赖关系

```text
Shell-3c_2
  ↓
4a_1 (状态切换：titlebar 直接调 SetWindowPos)
  ↓
4a_2 (持久化：bounds + resident_mode 落盘/恢复)
  ↓
Shell-5 (edge-reserved / AppBar)
```

### 整体目标

1. 标题栏菜单可切换"浮动置顶"模式
2. 置顶后窗口浮在其他窗口之上
3. 取消置顶后恢复正常层级
4. 隐藏→恢复后置顶状态保持
5. 重启后恢复上次的置顶状态和窗口位置

### 阶段产出

1. 标题栏菜单出现"浮动置顶"切换项
2. 置顶后其他窗口不覆盖便签
3. 关闭后重启，窗口位置和置顶状态恢复

## ⑤ 设定边界

### 本阶段范围

floating topmost 模式切换 + 浮动位置持久化。

### 不包括

1. 不做 AppBar / 贴边占位（那是 Shell-5）
2. 不做多显示器支持
3. 不做 titlebar 菜单以外的切换入口

### 分层归属

| 层 | 职责 |
|------|------|
| app/ | AppShellResidentMode 枚举扩展；App_RestoreFromTray 中检查 mode 恢复 topmost |
| platform/ | 不新增职责（SetWindowPos 由 titlebar 或 app 直接调） |
| storage/ | 落盘/恢复 resident_mode、last_floating_bounds |
| ui/titlebar | 菜单项点击 → 直接 SetWindowPos + 更新 mode |

### 文件落点

#### 预计修改文件

- `src/app/app.h` — AppShellResidentMode 新增 FLOATING_TOPMOST
- `src/app/app.c` — App_RestoreFromTray 中恢复 topmost；App_Init 启动恢复
- `src/ui/titlebar.c` — 菜单项 + 切换逻辑
- `src/storage/state_store.h` — 新增 last_floating_bounds
- `src/storage/state_store.c` — 读写新字段

#### 原则上不应修改

- `src/render/*`、`src/editor/*`、`src/core/*`、`src/platform/win32/window.c`

## ⑥ 落地方案

本阶段不涉及——技术路线由各子阶段的详细计划展开。

## ⑦ 验收标准

1. 标题栏菜单可切换浮动置顶
2. 置顶后其他窗口不覆盖便签
3. 取消置顶后恢复正常
4. 隐藏→恢复后置顶保持
5. 重启后窗口位置和置顶状态恢复
6. 分层归属与文件落点一致

## ⑧ 推进步骤

本文件为纲领文件，推进步骤由各子阶段的详细计划提供。
