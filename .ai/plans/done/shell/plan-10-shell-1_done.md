# Shell-1 - 便签壳层：定义自绘窗体、标题栏按钮面与边框合同（后续）

## 阶段摘要

把系统从“普通 Win32 窗口 + 最小编辑器”推进到“已经具备便签外观基线的自绘窗体壳层”。

> 本阶段严格承接：`../plan.md`、`../../docs/design/architecture.md`、已完成的 `phase-6.md` 到 `phase-9.md`

## 当前执行方式

Shell-1 不再作为一次性大阶段直接推进，当前按“已完成保留 + 后续继续”的方式拆成 5 个子阶段：

1. `phase-shell-1a_done.md`：壳层状态与标题栏命令模型（已完成，保留）
2. `phase-shell-1b_done.md`：最小 non-client 骨架与顶区拖拽占位（已完成，保留）
3. `phase-shell-1c_done.md`：Frameless 窗体切换与客户区重建（已完成，冻结）
4. `phase-shell-1c-repair-1.md`：修复旧持久化状态把 frameless 回退为系统窗体
5. `phase-shell-1d.md`：标题栏组件与边框视觉基线

当前执行时不应再回写 `1c_done`，而应按 `1c_done -> 1c-repair-1 -> 1d` 顺序继续推进。

## 工作核心

1. 先把壳层切到稳定的 frameless / client-area 基线，再接标题栏与边框视觉
2. 把标题栏从单一 caption，提升为壳层命令面
3. 明确“边框视觉”和“边框交互”的合同
4. 为后续拖拽缩放、托盘后台、桌面常驻模式建立统一壳层入口

## 阶段目标

1. 去掉系统标题栏和系统边框，完成 frameless 窗体切换与客户区重建
2. 建立标题栏、边框、正文三块区域的稳定视觉与布局关系
3. 建立标题栏按钮体系，而不是只放零散按钮
4. 建立边框合同，明确哪些区域只负责视觉，哪些区域后续承担 resize 语义
5. 在 `app` 中建立最小壳层状态与壳层命令面
6. 为后续 Shell-2 / Shell-3 / Shell-4 / Shell-5 预留统一入口和最小持久化字段

## 为什么要做这个

1. 如果未完成的 frameless 主体继续混在 `_done` 文件里，后续 AI 会错误地把阶段边界判错，导致不可追溯和重复返工
2. 如果没有先把 frameless / client-area 基线单独收口，标题栏和边框视觉后续仍会挂在不稳定的壳层结构上
3. 这个阶段必须先把壳层基线定清，后续阶段才能在稳定结构上继续叠加系统行为

## 本阶段产出

本阶段完成后，应看到下面这些结果：

1. 主窗口已不再显示系统标题栏 / 系统边框
2. frameless / client-area 主链稳定，可供后续视觉层继续承接
3. 标题栏不再只是“能显示标题”，而是开始承载壳层按钮面
4. 标题栏、边框、正文区边界关系已经稳定
5. 后续拖拽、缩放、托盘、常驻模式都有清楚的承接点

## 当前切入点

进入本阶段前，系统已经具备下面这些前提：

1. `app / platform / render / ui / editor / core / storage` 的基础分层已经落地
2. 单窗口、单便签、基础输入链已经可用
3. 保存安全主线 `Phase 6-9` 已完成：
   - dirty / version 模型
   - autosave
   - atomic save
   - crash recovery
4. 当前系统仍然缺少：
   - 完整 frameless 窗体切换与客户区重建主链
   - 标题栏组件本体
   - 标题栏按钮面布局
   - 边框视觉基线
   - 正文区相对标题栏 / 边框的稳定布局合同

因此，本阶段不再讨论：

1. 保存安全语义怎么定义
2. 编辑器能力如何继续扩展
3. Markdown 交互如何推进
4. 拖拽 / 缩放 / 托盘 / AppBar 的完整实现

本阶段只讨论：

1. 已完成部分如何保留为 Shell-1 基线
2. `Shell-1c-repair-1` 如何修复旧持久化状态导致的 frameless 回退
3. `Shell-1d` 如何承接标题栏按钮面与边框视觉
4. `Shell-1d` 与 `Shell-2a / 2b` 的边界切分

## 本阶段范围

当前 Shell-1 只包含：

1. 保留 `Shell-1a_done` 的最小壳层状态模型和命令模型
2. 保留 `Shell-1b_done` 的最小 non-client 骨架与顶区拖拽占位
3. 在 `Shell-1c_done` 中保留 frameless 窗体切换与客户区重建主体
4. 在 `Shell-1c-repair-1` 中修复启动期兼容与状态迁移问题
5. 在 `Shell-1d` 中建立标题栏、边框、正文三块区域的稳定布局切分
6. 建立标题栏按钮体系定义
7. 建立边框合同定义：
   - 视觉边框宽度
   - 后续 resize 交互预留宽度
   - 边框与正文留白关系

## 本阶段不做

1. 再做一次 `Shell-1a` 的状态建模
2. 回头改写 `Shell-1c_done` 已冻结的主体定义
3. 完整拖拽 / 缩放 hit test
4. 托盘后台生命周期
5. 浮动置顶 / AppBar 贴边占位
6. 多窗口壳层协调
7. 复杂壳层动画、阴影、磨砂、Acrylic 等视觉增强
8. 标题栏命令插件化
9. 编辑器能力扩展

## 分层归属

### `platform/`

- 负责 Win32 窗体样式切换
- 负责客户区 / 非客户区基础消息适配
- 负责最小非客户区取消和客户区重建
- 不负责标题按钮业务语义

### `app/`

- 持有最小壳层状态
- 负责把标题栏按钮命令收束成统一壳层命令
- 负责协调壳层状态与 `storage` 的读写
- 不直接写具体绘制代码

### `ui/`

- 定义标题栏、边框、正文布局
- 定义标题栏按钮体系和命令分组
- 定义 hover / pressed / active 的界面语义
- 不直接切换 Win32 窗体样式

### `render/`

- 负责标题栏、边框、按钮视觉绘制
- 只吃 `ui` 提供的布局和状态结果
- 不理解托盘 / topmost / AppBar 这些系统语义

### `storage/`

- 为壳层状态提供最小持久化落点
- 当前阶段只接最小字段，不提前引入完整后台 / 常驻配置

### `editor/`

- 只接收正文客户区尺寸变化
- 不知道标题栏和边框是否来自系统还是来自自绘壳层

### `core/`

- 当前阶段原则上不参与

## 文件落点

### 本阶段预计修改文件

- `src/platform/win32/window.c`
- `src/platform/win32/window.h`
- `src/app/app.h`
- `src/app/app.c`
- `src/ui/editor_view.h`
- `src/ui/editor_view.c`
- `src/render/render.h`
- `src/render/render.c`
- `src/storage/state_store.h`
- `src/storage/state_store.c`

当前阶段大概率需要新增：

- `src/ui/titlebar.h`
- `src/ui/titlebar.c`

如平台非客户区逻辑需要单独收口，可接受新增：

- `src/platform/win32/nonclient.h`
- `src/platform/win32/nonclient.c`

### 本阶段原则上不应修改

- `src/editor/editor.h`
- `src/editor/editor.c`
- `src/core/*`
- `src/storage/note_store.h`
- `src/storage/note_store.c`
- `src/storage/storage_write.c`

## 技术路线

### `src/app/app.h`

- 新增最小壳层状态读取 / 变更接口
- 壳层状态当前至少应能表达：
  - 是否使用自绘壳层
  - 标题栏高度
  - 边框视觉宽度
  - 边框交互预留宽度
  - 当前激活的壳层命令
  - 标题栏按钮面配置

### `src/app/app.c`

- 新增壳层状态结构，例如：
  - `use_custom_chrome`
  - `titlebar_height`
  - `frame_visual_thickness`
  - `frame_resize_thickness`
  - `active_shell_command`
  - `titlebar_command_set`
- 启动时从 `storage` 读取最小壳层配置
- 运行时接收标题栏命令并统一分发
- 保证标题栏按钮行为仍通过 `app` 编排，而不是 `ui` 直接调平台 API

### `src/platform/win32/window.c` / `window.h`

- 切换 Win32 样式，去掉系统 caption / frame
- 建立最小客户区计算与重绘配合
- 暂时只完成“系统框架移除”和“窗口仍可正常显示”两件事
- 不在本阶段把完整 resize / drag 行为硬塞进 `WndProc`

### `src/ui/titlebar.h` / `titlebar.c`

- 新建标题栏组件
- 输出：
  - 标题栏矩形
  - 按钮矩形
  - 标题文本区域
  - 预留命令区域
  - 基础视觉状态
- 标题栏按钮至少应分成：
  - 窗口控制类
  - 壳层模式类
  - 后台 / 托盘类预留

### `src/ui/editor_view.h` / `editor_view.c`

- 从“整块客户区”切换到“标题栏和边框以内的正文区”
- 收紧正文区布局，避免标题栏高度和边框宽度硬编码散落

### `src/render/render.h` / `render.c`

- 提供标题栏、边框和按钮所需的基础绘制能力
- 如现有 `fill rect / draw text` 足够，优先复用
- 若确需新增绘制原语，只新增通用原语，不新增业务语义 API

### `src/storage/state_store.h` / `state_store.c`

- 为最小壳层状态预留落盘字段
- 当前阶段建议只落：
  - `use_custom_chrome`
  - `titlebar_height`
  - `frame_visual_thickness`
  - 后续壳层模式预留枚举值
- 不提前把 Shell-3 / Shell-4 / Shell-5 的所有状态一次性写满

## 标题栏按钮体系

当前阶段建议先把标题栏按钮体系收束到下面三组，而不是只放孤立按钮：

```text
TitlebarCommandGroup
├── window-controls      # 关闭、最小化、还原等
├── shell-modes          # 置顶、贴边、常驻等后续入口
└── background-actions   # 隐藏到托盘、恢复等后续入口
```

说明：

1. 当前阶段不要求这些按钮全部可用
2. 当前阶段要求先把“按钮体系”建出来
3. 这样后续按钮扩展不会打散标题栏结构

## 边框合同

当前阶段需要把“边框”拆成两层语义：

```text
ShellFrameContract
├── visual_thickness      # 用户看到的边框厚度
├── resize_thickness      # 后续可参与 resize hit test 的热区厚度
└── content_inset         # 正文区相对边框的留白
```

规则：

1. 视觉边框和交互边框可以一致，也可以不同
2. 当前阶段先把合同定清，不要求本阶段就完成全部 resize 命中
3. 正文区不能直接贴死到窗体边缘

## 主链路

当前阶段最核心链路固定为：

```text
程序启动
-> app 初始化最小壳层状态
-> platform 创建 frameless 窗体
-> ui 计算标题栏 / 边框 / 正文布局
-> render 绘制标题栏、边框和正文区
-> 用户点击标题栏按钮
-> ui 命中按钮
-> app 收束壳层命令
```

## 完成标准

本阶段结束时，至少必须满足：

1. `Shell-1a_done` 与 `Shell-1b_done` 被明确保留为 Shell-1 已完成前半段
2. `Shell-1c_done -> Shell-1c-repair-1 -> Shell-1d` 的顺序明确，且当前切入点固定为 `Shell-1c-repair-1`
3. frameless / client-area 基线、启动兼容修复、标题栏、边框、正文区三者关系稳定
4. 标题栏按钮面已经成型，不再只是零散按钮
5. 边框视觉与后续交互合同已经明确
6. 后续 Shell-2 / Shell-3 / Shell-4 / Shell-5 不再重复实现 Shell-1 已完成内容

## 计划的最后一步

1. `git add` 当前阶段修改过的 shell 计划文件
2. `git commit` 使用规范提交说明记录这次阶段计划调整
3. `git push` 把阶段计划更新提交到远端仓库
4. 如果当前计划确认已完成，把对应计划文件重命名为追加 `_done` 后缀的文件名
