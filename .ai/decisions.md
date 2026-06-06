# Decisions

> 本文件记录"为什么这样定"。对应执行步骤与阶段安排见：`./plans.md`

## 2026-05-25 - Render 作为基础绘制后端

关联计划位置：

- `./plans.md` → `当前方案收束` → `当前对 render 的定义`
- `./plans.md` → `Phase 2 - 打通显示链路`

### 决策

当前阶段将 `render/` 定义为**基础绘制后端**，只负责：

- 帧生命周期
- 清背景
- 画矩形 / 边框 / 文字
- 封装底层绘图 API

### 不负责

- 编辑器语义
- 标题栏语义
- Markdown 语义
- 输入事件处理

### 影响

- `ui` 组件直接调用 `render` 的基础接口完成绘制
- `platform` 不直接承担界面绘制细节
- 后续富文本渲染也应通过 `ui/editor_view` 组织，再调用 `render`

### 当前结论

采用：

```text
platform -> app -> ui -> render
```

而不是让 `render` 反向理解业务组件。

## 2026-05-25 - Windows 渲染直接采用 Direct2D + DirectWrite

关联计划位置：

- `./plans.md` → `当前方案收束`
- `./plans.md` → `Phase 2 - 打通显示链路`

### 决策

当前 Windows 渲染路径直接采用 `Direct2D + DirectWrite`。

### 不采用

- 不新增 GDI 作为新的过渡渲染路径
- 不在 `ui/` 或 `platform/` 中直接散落 Direct 绘制逻辑

### 影响

- `render/` 从第一版开始就需要有明确的初始化、释放、resize、begin/end frame 生命周期
- `ui/` 通过 `render/` 间接使用 Direct 绘制能力
- `WndProc` 中的绘制细节需要逐步搬离到 `app -> ui -> render` 主链

### 当前结论

采用：

```text
platform -> app -> ui -> render(Direct2D/DirectWrite)
```

## 2026-05-27 - Shell-1d：标题栏/边框由 ui/titlebar 组件绘制，render 层不新增原语

关联计划位置：

- `./plans.md` → `当前阶段焦点`
- `.ai/plans/plan-10-shell-1d_done.md` → `技术路线`

### 决策

Shell-1d 中标题栏和边框视觉由 `ui/titlebar` 组件统一承接：

- `ui/titlebar` 负责标题栏布局计算、标题栏背景/按钮绘制、窗口边框绘制
- `render` 层**不新增任何绘制原语**，`Render_FillRect` + `Render_DrawText` + `Render_PushClip/PopClip` 已覆盖全部需求
- `editor_view` 不做布局改动，其正文区矩形已由 `nonclient.c` 的 `ComputeContentRect` 正确计算，经 `App_RequestClientRebuild → EditorView_OnClientAreaChanged` 可靠传递
- 标题栏按钮在当前阶段仅为**视觉状态**，不处理命中事件（Shell-2a 承接交互命中）

### 不采用

- 不把边框/标题栏绘制放入 render 层（保持 render 为无业务语义的基础后端）
- 不新增 titlebar 专用的 render 原语（如 `Render_DrawRoundedRect`）

### 影响

- `app.c` 只有一处改动：在 `App_OnPaint` 的 `Render_Clear` 与 `EditorView_Draw` 之间插入 `Titlebar_Draw`
- `ui/titlebar` 对 `render` 的调用与 `ui/editor_view` 对 `render` 的调用采用**相同的接口集**，没有引入 render 层不理解的新原语
- 后续 Shell-2a 的按钮命中、Shell-2b 的拖拽缩放都可以在这个布局基线上叠加，无需回头修改视觉层

## 2026-06-05 - Plans 11/12/13 文档重组：测试用例写入子阶段文件

关联计划位置：

- `./ai/plans/plan-11-unified-config-entry.md`
- `./ai/plans/plan-12-index.md`
- `./ai/plans/plan-13-index.md`

### 决策

将 Plans 11/12/13 的测试验证部分从独立测试文件迁入各子阶段计划文件中。

格式标准：每个子阶段文件末尾包含 `## 测试用例` 节（前置条件 + 自动化检查 + 手工验证 + GATE 通过条件），测试追踪器（plan-N-test.md）只做看板，引用子阶段文件的 anchor，不复制测试细节。

### 变更内容

- 修正 9 个文件标题（`phase-11|12` → `Plan-12|13`）
- Plan 11 的 8 个 `_done` 文件：`## 验证要点` → `## 测试用例` 升级
- 11a + 11b 合并为统一 GATE 1-2
- Plan 13 测试编号修复（`12-x` → `13-x`）
- 新建 `plan-12-test.md`、`plan-13-test.md`、`plan-13-index.md`
- `done/` 下 00/01 文件追加 GATE 通过条件

### 不改变

- `_done` 仅表示代码实现完成，测试验证尚未执行
- 不修改任何 `src/` 代码文件
- 不删除已有文件

## 2026-06-06 - 废弃旧 Plan 11 (Config)，重新规划 struct 配置体系

关联计划位置：

- `./ai/plans/archive/superseded/plan-11-unified-config-entry_superseded.md`
- 新 Plan 11 将在 `.ai/plans/` 创建

### 决策

旧 Plan 11（平面 key=value Config 层）标记为废弃。

重新设计为三层结构的配置体系：

```c
struct system_info {
    // 启动时从 Windows API 采集，不持久化
    int dpi;
    int scale_pct;
    RECT work_area;       // SPI_GETWORKAREA
    int screen_width;
    int screen_height;
};

struct dn_global {
    // 持久化到 state.ini
    int resident_mode;
    int dock_edge;
    int dock_thickness;
    int presence_state;
    int last_floating_left, last_floating_top;
    int last_floating_width, last_floating_height;
    int titlebar_height;
    int frame_visual_thickness;
};

struct dn_note {
    // 每个便签独立（当前仅一页，先搭结构）
    wchar_t path[MAX_PATH];
    // 后续扩展
};
```

### 理由

1. 平面 key=value 只存 int，无法表达 RECT 等多字段数据
2. 无编译期类型检查，字符串 key 拼错不报错
3. 运行时信息（DPI、屏幕尺寸）与持久化配置混在一起，来源不清
4. 不支持多便签独立配置

### 影响

- `src/config/config.c` / `config.h` 保留但标记废弃，不再扩展新字段
- `src/utils/ini2arr.c` / `ini2arr.h` 保留，作为底层 INI 读写工具
- 新增统一全局结构体接管运行时信息 + 持久化配置
- 所有硬编码 240 改为基于屏幕比例计算
- 旧文件已移至 `done/` 加 `_superseded` 后缀
