# Shell-Index - Shell 主线统一进度管理（后续）

## 阶段摘要

把 Shell 主线从"几个大而泛的阶段描述"推进到"可按子阶段连续落地、可统一跟踪进度的执行索引"。

## 阶段目标

1. 为 Shell 主线建立统一进度索引
2. 把 Shell 主线拆成更小的可执行子阶段
3. 明确子阶段依赖关系与推荐推进顺序
4. 给后续实现提供单一进度入口，而不是在多个大阶段文档间来回跳转

## 为什么要做这个

1. 当前 Shell 计划如果只有几个大阶段，执行时仍然太粗，容易再次失控膨胀
2. 没有统一进度入口时，后续推进会在多个大阶段文档之间来回跳转，状态难维护
3. 先建立总索引，才能让后续每个 Shell 子阶段都变成真正可管理的落地点

## 阶段产出

1. Shell 主线有统一索引文件
2. Shell 子阶段有统一状态表
3. 当前该推进哪个子阶段一目了然
4. 各子阶段之间的依赖关系清楚

## 本阶段范围

1. Shell 子阶段状态矩阵
2. Shell 子阶段依赖关系
3. Shell 当前推荐切入点
4. Shell 文档读取顺序

## 本阶段不做

1. 直接替代各子阶段文档
2. 在本文件里重复写每个子阶段的全部技术细节
3. 代替 `.ai\plan.md` 的全局阶段索引

## 分层归属

### `platform/`

- 本文件不承载实现职责

### `app/`

- 本文件不承载实现职责

### `ui/`

- 本文件不承载实现职责

### `render/`

- 本文件不承载实现职责

### `editor/`

- 本文件不承载实现职责

### `core/`

- 本文件不承载实现职责

### `storage/`

- 本文件不承载实现职责

## 文件落点

### 本阶段预计修改文件

- `.ai/plan.md`
- `.ai/phases/phase-shell-index.md`
- `.ai/phases/phase-shell-1a_done.md`
- `.ai/phases/phase-shell-1b_done.md`
- `.ai/phases/phase-shell-1c_done.md`
- `.ai/phases/phase-shell-1c-repair-1.md`
- `.ai/phases/phase-shell-1d.md`
- `.ai/phases/phase-shell-2a.md`
- `.ai/phases/phase-shell-2b.md`
- `.ai/phases/phase-shell-3a.md`
- `.ai/phases/phase-shell-3b.md`
- `.ai/phases/phase-shell-3c.md`
- `.ai/phases/phase-shell-4a.md`
- `.ai/phases/phase-shell-5a.md`
- `.ai/phases/phase-shell-5b.md`

### 本阶段原则上不应修改

- `src/*`

## 统一状态表

| 子阶段 | 状态 | 主题 | 依赖 | 详细文件 |
|------|------|------|------|------|
| Shell-1a | ✅ 已完成 | 壳层状态与标题栏命令模型 | - | `./done/phase-10-shell-1a_done.md` |
| Shell-1b | ✅ 已完成 | 最小 non-client 骨架与顶区拖拽占位 | Shell-1a | `./done/phase-10-shell-1b_done.md` |
| Shell-1c | ✅ 已完成 | Frameless 窗体切换与客户区重建 | Shell-1a, Shell-1b | `./done/phase-10-shell-1c_done.md` |
| Shell-1c_repair-1 | ✅ 已完成 | 修复旧持久化状态把 frameless 回退为系统窗体 | Shell-1c | `./done/phase-10-shell-1c_repair-1_done.md` |
| Shell-1d | ✅ 已完成 | 标题栏组件与边框视觉基线 | Shell-1c_repair-1 | `./done/phase-10-shell-1d_done.md` |
| Shell-1e | ✅ 已完成 | 通用按钮组件与窗口白色边框修复 | Shell-1d | `./done/phase-10-shell-1e_done.md` |
| Shell-2a | ✅ 已完成 | 菜单按钮 + 弹出菜单 + 标题栏拖拽 | Shell-1e | `./done/phase-10-shell-2a_done.md` |
| Shell-2a_repair-1 | ✅ 已完成 | 隔离验证：隐藏按钮确认组件通道 | Shell-2a | `./done/phase-10-shell-2a_repair-1_done.md` |
| Shell-2a_repair-2 | ✅ 已完成 | 可见性修复：菜单按钮颜色 + 文字标识 | Shell-2a_repair-1 | `./done/phase-10-shell-2a_repair-2_done.md` |
| Shell-2a_repair-3 | ✅ 已完成 | 回归验证：汉堡图标 + 功能验证 | Shell-2a_repair-2 | `./done/phase-10-shell-2a_repair-3_done.md` |
| Shell-2a_repair-4 | ✅ 已完成 | render对齐扩展：Render_DrawTextCentered | Shell-2a_repair-3 | `./done/phase-10-shell-2a_repair-4_done.md` |
| Shell-2a_repair-5 | ✅ 已完成 | button改用Render_DrawTextCentered居中绘制 | Shell-2a_repair-4 | `./done/phase-10-shell-2a_repair-5_done.md` |
| Shell-2a_repair-6 | ✅ 已完成 | titlebar菜单按钮配色调整 | Shell-2a_repair-5 | `./done/phase-10-shell-2a_repair-6_done.md` |
| Shell-2a_repair-7 | ✅ 已完成 | hover坐标修正+链路验证 | Shell-2a_repair-6 | `./done/phase-10-shell-2a_repair-7_done.md` |
| Shell-2a_repair-8 | ✅ 已完成 | 鼠标移出窗口state残留修复 | Shell-2a_repair-7 | `./done/phase-10-shell-2a_repair-8_done.md` |
| Shell-2a_repair-9 | ✅ 已完成 | 恢复hover色为暖黄色 | Shell-2a_repair-8 | `./done/phase-10-shell-2a_repair-9_done.md` |
| Shell-2b_1 | ✅ 已完成 | 默认窗口240x388 + 编辑区背景铺满 | Shell-2a_repair-9 | `./done/phase-10-shell-2b_1_done.md` |
| Shell-2b_1a | ✅ 已完成 | 窗口初始位置设为屏幕右上角 | Shell-2b_1 | `./done/phase-10-shell-2b_1a_done.md` |
| Shell-2b_2 | ✅ 已完成 | 窗口四边缩放 | Shell-2b_1a | `./done/phase-10-shell-2b_2_done.md` |
| Shell-2b_3 | ✅ 已完成 | 窗口四角缩放 | Shell-2b_2 | `./done/phase-10-shell-2b_3_done.md` |
| Shell-2b_4 | ✅ 已完成 | resize后布局回归验证 | Shell-2b_3 | `./done/phase-10-shell-2b_4_done.md` |
| Shell-3a_1 | ✅ 已完成 | 托盘图标创建与销毁 | Shell-2b_4 | `./done/phase-10-shell-3a_1_done.md` |
| Shell-3a_2 | ✅ 已完成 | 托盘消息管线（左键/右键菜单） | Shell-3a_1 | `./done/phase-10-shell-3a_2_done.md` |
| Shell-3b | ✅ 已完成 | 任务栏 / 托盘 presence 矩阵与显隐链 | Shell-3a_2 | `./done/phase-10-shell-3b_done.md` |
| Shell-3c_1 | ✅ 已完成 | 关闭策略（隐藏到托盘vs退出） | Shell-3b | `./done/phase-10-shell-3c_1_done.md` |
| Shell-3c_2 | ✅ 已完成 | 状态持久化（presence+resident mode） | Shell-3c_1 | `./done/phase-10-shell-3c_2_done.md` |
| Shell-4a_1 | ✅ 已完成 | floating topmost 状态机 | Shell-3c_2 | `./done/phase-10-shell-4a_1_done.md` |
| Shell-4a_2 | ✅ 已完成 | floating topmost 持久化 | Shell-4a_1 | `./done/phase-10-shell-4a_2_done.md` |
| Shell-5a | ❌ 已回滚 | AppBar 注册与贴边模型（存在死区Bug） | Shell-4a_2 | `./phase-10-shell-5a.md` |
| Shell-5a_repair-5-a | 📋 方案A已选 | 启动与托盘恢复（立即注册方案）— 已拆分为子计划 | Shell-5a | `./phase-10-shell-5a_repair-5-a.md` |
| Shell-5a_repair-5-a-1 | ✅ 已完成 | 启动时立即注册 AppBar | repair-5-a | `./done/phase-10-shell-5a_repair-5-a-1_startup-register_done.md` |
| Shell-5a_repair-5-a-2 | ✅ 已完成 | 从托盘恢复（已改为不重建贴边） | repair-5-a-1 | `./done/phase-10-shell-5a_repair-5-a-2_restore-from-tray_done.md` |
| Shell-5a_repair-5-a-3 | ✅ 已完成 | 浮动→贴边：保存 last_floating + 取消 topmost | repair-5-a-1 | `./done/phase-10-shell-5a_repair-5-a-3_switch-from-floating_done.md` |
| Shell-5a_repair-5-a-4 | ✅ 已完成 | 新增 release_on_hide/drag state 字段 | - | `./done/phase-10-shell-5a_repair-5-a-4_release-state-fields_done.md` |
| Shell-5a_repair-5-a-5 | ✅ 已完成 | 日志前缀与参数校验工具 | - | `./done/phase-10-shell-5a_repair-5-a-5_logging-helpers_done.md` |
| Shell-5a_repair-5-a_debug_record | ✅ 已解决 | AppBar 死区残留问题（已知 Windows 行为，记录备查） | repair-5-a-3 | `./done/phase-10-shell-5a_repair-5-a_debug_record_done.md` |
| Shell-5a_repair-5-b | ✅ 已归档 | 由 repair-5-a-3 覆盖 | Shell-4a_2 | `./done/phase-10-shell-5a_repair-5-b_done.md` |
| Shell-5a_repair-5-c | ✅ 已归档 | hide/drag 释放已实现，config 字段未接入（计划偏差） | repair-5-a-4 | `./done/phase-10-shell-5a_repair-5-c_done.md` |
| Shell-5b | ✅ 已完成 | 工作区重同步与 AppBar 异常恢复（含节流修复反馈循环） | Shell-5a | `./done/phase-10-shell-5b_done.md` |
| Shell-5c | ✅ 已完成 | 常驻模式状态提示（托盘 hover + 菜单 ✓） | Shell-5b | `./done/phase-10-shell-5c_done.md` |
| Shell-5d | ✅ 已完成 | 标题栏状态指示灯（橙色/蓝色圆点） | Shell-5c | `./done/phase-10-shell-5d_done.md` |

## 推荐推进顺序

### 已完成（Shell-1a → Shell-3c_2）

```text
Shell-1a → Shell-1b → Shell-1c → Shell-1c_repair-1
→ Shell-1d → Shell-1e
→ Shell-2a → Shell-2a_repair-1~9
→ Shell-2b_1 → Shell-2b_1a → Shell-2b_2 → Shell-2b_3 → Shell-2b_4
→ Shell-3a_1 → Shell-3a_2 → Shell-3b
→ Shell-3c_1 → Shell-3c_2
═══ 全部已完成 ═══
```

### 剩余待推进

```text
Shell-5a_repair-5-c (部分完成：hide/drag 释放已实现)
  ↓
Shell-5c (常驻模式状态提示)
```

## 当前推荐切入点

- **Shell-5a_repair-5-c** 剩余工作：把 `release_on_hide_mode` / `release_on_drag_mode` 字段接入 `App_HideToTray` 和 `App_OnEndDrag`（当前 hide 是硬编码无条件释放）

## 读取规则

进入 Shell 主线时，建议按下面顺序读取：

1. `../plan.md`
2. `./phase-10-shell-index.md`
3. 当前 ready 的 `./phase-10-shell-Xx.md` 或 `./done/phase-10-shell-Xx_done.md`
4. 需要核对边界时，再读 `../../docs/design/architecture.md`

## 完成标准

1. Shell 主线已经拆成可落地子阶段
2. 子阶段依赖关系清楚
3. 当前 ready 子阶段清楚
4. 后续 Shell 推进不再依赖口头记忆

## Shell 全局状态关系附录

### 1. 状态轴拆分

Shell 主线后续实现时，至少按下面 3 条状态轴拆开，而不是混成一个大状态：

1. `ShellPresenceState`
   - `visible_front`
   - `hidden_to_tray`
   - `exiting`
2. `ShellResidentMode`
   - `none`
   - `floating_topmost`
   - `edge_reserved`
3. `ShellDockState`
   - `edge`
   - `thickness`
   - `last_floating_bounds`

### 2. 互斥关系

1. `ShellResidentMode` 三个值互斥，同一时刻只能有一个有效 resident mode
2. `ShellPresenceState` 三个值互斥，同一时刻只能有一个有效 presence state
3. `ShellDockState` 只在 `resident_mode = edge_reserved` 时有效，其他模式下只能保留为恢复信息，不能继续驱动工作区保留

### 3. 可叠加关系

1. `visible_front` 可以和：
   - `none`
   - `floating_topmost`
   - `edge_reserved`
   组合
2. `hidden_to_tray` 可以和 resident mode 记录同时存在，但平台侧不能继续保留无效系统资源：
   - `floating_topmost` 隐藏后只保留 resident mode 语义，恢复时重新应用 topmost
   - `edge_reserved` 隐藏后必须先释放 AppBar 工作区保留，再保留 resident mode 与 dock state 供恢复使用

### 4. 从 tray 恢复时的 resident mode 规则

1. 先恢复 `presence_state = visible_front`
2. 再根据隐藏前保存的 resident mode 恢复壳层模式：
   - 若为 `none`，仅恢复普通前台窗口
   - 若为 `floating_topmost`，恢复前台后重新应用 topmost
   - 若为 `edge_reserved`，恢复前台后重新注册 AppBar，并按 dock state 重建保留区
3. 恢复顺序必须固定为：
   - 先恢复可见性
   - 再恢复 resident mode
   - 最后恢复相关 bounds / dock state

### 5. 从 edge_reserved 切回其他模式的优先级

1. `edge_reserved -> floating_topmost`
   - 先注销 AppBar
   - 再恢复 `last_floating_bounds`
   - 最后应用 topmost
2. `edge_reserved -> none`
   - 先注销 AppBar
   - 再恢复 `last_floating_bounds`
   - 最后保持普通窗口层级
3. 不允许在 AppBar 仍占用工作区时直接跳成 `floating_topmost` 或 `none`

## 计划编写标准

所有计划文件必须遵循 `.ai/plans-standard.md` 定义的模板结构。

**强制要求（自本节发布起生效）：**
- 每个计划必须有第⑤节「测试计划」，覆盖正常路径、边界、错误、回归、集成 5 个分组
- 每个测试用例必须有具体的预期日志输出和前置条件
- 验收标准禁止使用模糊用语，必须替换为可观测的具体指标
- 已有计划文件应在本阶段内补全测试计划，新计划必须带测试计划才能进入实施
