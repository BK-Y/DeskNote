# Shell-2b — 便签壳层：窗口布局基线 + 缩放交互（后续）

## 阶段摘要

把系统从"标题栏菜单按钮 + 拖拽已完备"推进到"窗口布局符合便签定位、四边四角可缩放、resize 后布局稳定"的状态。

## 承接关系

前置阶段（已完成）：

- `Shell-2a_done` — 菜单按钮 + 弹出菜单 + 标题栏拖拽
- `Shell-2a_repair-1~9_done` — 菜单按钮视觉修复全链路

本阶段直接消费 Shell-2a 的产出：

- 菜单按钮、弹出菜单、拖拽
- Button 组件 + Render_DrawTextCentered
- hover/pressed 状态驱动

## 工作核心

1. 窗口默认尺寸从 800x600 调整为 360x500（便签尺寸）
2. 编辑区黄色背景铺满整个内容区，不留灰色边缘
3. 窗口四边/四角可缩放
4. resize 后标题栏、边框、正文区布局稳定

## 整体目标

1. 默认窗口 360x500，启动即符合便签大小预期
2. 黄色编辑区铺满客户区，视觉上"从边到边都是便签纸"
3. 用户可从窗口边缘和角落拖拽缩放
4. resize 过程中和 resize 后 UI 不出现错位或白块

## 约束要求

1. 编辑区背景铺满不影响标题栏和边框绘制——editor_rect 的 margin=0，但 text_rect 保留左右 padding（防止文字贴边）
2. 缩放热区判断优先级：四角 > 四边 > 标题栏拖拽
3. resize 后通过 `App_OnResize` → `Render_Resize` + `Platform_RebuildClientArea` 更新布局，不单独处理绘制
4. 不修改 render 层

## 子阶段列表

| 子阶段 | 状态 | 主题 |
|--------|------|------|
| Shell-2b_1 | 已完成 | 默认窗口 240x388 + 编辑区背景铺满 |
| Shell-2b_1a | 已完成 | 窗口初始位置设为屏幕右上角 |
| Shell-2b_2 | 已完成 | 窗口四边缩放 |
| Shell-2b_3 | 已完成 | 窗口四角缩放 |
| Shell-2b_4 | 已完成 | resize 后布局回归验证 |

### 依赖关系

```text
Shell-2a_repair-9_done
  ↓
Shell-2b_1_done (默认尺寸 + 背景铺满)
  ↓
Shell-2b_1a_done (初始位置)
  ↓
Shell-2b_2 (四边缩放，排除四角)
  ↓
Shell-2b_3 (四角缩放)
  ↓
Shell-2b_4 (resize 回归验证)
  ↓
Shell-3a_1 (托盘图标)
```

### 各子阶段边界

| 能力 | 归属 | 说明 |
|------|------|------|
| 默认窗口 360x500 | 2b_1 | `CreateWindowExW` 参数 + editor_view margin=0 |
| 黄色背景铺满 | 2b_1 | `EditorView_CalculateLayout` 中 margin/top_margin 改为 0 |
| 四边缩放 | 2b_2 | `Platform_Nonclient_HandleNCHitTest` 返回 HTLEFT/HTRIGHT/HTTOP/HTBOTTOM |
| 四角缩放 | 2b_3 | 四角命中优先于四边 |
| resize 回归 | 2b_4 | 手动验证，不修改代码 |

## 读取规则

进入 Shell-2b 时：

1. 先读 `plan.md` 确认当前焦点
2. 再读 `phase-shell-2b.md`（本文件）了解整体目标和约束
3. 最后只读当前焦点的子阶段
4. 核对边界时读 `docs/design/architecture.md`

## 计划的最后一步

1. `git add` 当前阶段修改过的文件
2. `git commit` 使用规范提交说明
3. `git push`
