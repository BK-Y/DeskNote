# DeskNote 架构设计文档

> **学习项目说明：** 本项目同时也是 C 语言和 Win32 API 的学习计划。
> 没有明确的截止日期或硬性需求，每个阶段按自己的节奏推进。
> 文档中列出的估时和阶段顺序只是参考方向，不是承诺。

本文档分三部分：
1. **遗留实现**（历史快照）—— 旧版 Win32 + GDI 代码结构
2. **当前重构约定**（当前有效）—— 当前已经落地或正在继续收紧的模块边界与技术决策
3. **远期目标架构** —— 后续持续演进方向

## 当前状态快照（2026-05）

当前仓库里的实际状态已经不是“无编辑器 / 待接入存储”的阶段，而是：

- 已完成 `app / platform / render / ui / editor / core / storage` 分层拆分
- 已完成最小文本输入链与基础浏览
- 已完成 DirectWrite 光标测量
- 已完成系统光标与 IME 跟随
- 已完成 `%LOCALAPPDATA%\DeskNote\state.ini` + `note.md` 持久化
- 已完成保存安全主线 `Phase 6-9`（dirty / autosave / atomic save / crash recovery）
- 下一阶段主线切换为“便签窗口壳层”：自绘窗体、托盘后台、桌面常驻模式

> 当前执行计划不再维护在 `docs\plan\`，而是维护在 `.ai\plan.md` 与 `.ai\phases\`。

---

## 第一部分：遗留实现（v0.1.0 之前）

### 技术栈

| 维度 | 内容 |
|------|------|
| 语言 | C99 |
| UI 层 | Win32 API（GDI 已移除，后续重构为 Direct2D + DirectWrite）|
| Markdown 解析 | md4c（第三方 C 库，submodule）|
| 存储 | `%APPDATA%\DeskNote\` 目录 + JSON 配置 |
| 构建 | CMake 3.20+ + MinGW-w64 GCC |
| 产物 | 单文件 `desknote.exe`，绿色免安装 |

### 架构分层（当时实际）

```
┌──────────────────────────────────────────────┐
│                  消息循环                      │
│    GetMessage → TranslateMessage → Dispatch   │
├──────────────────────────────────────────────┤
│               WndProc（主窗口过程）             │
│   WM_PAINT → Titlebar_Draw（GDI，待重构）     │
│   WM_LBUTTONDOWN → HitTest / 拖拽 / 按钮      │
│   WM_NCHITTEST → Window_ResizeHitTest         │
│   WM_MOUSEMOVE/LEAVE → 悬停 / 自动隐藏        │
│   WM_RBUTTONDOWN → 右键菜单                   │
│   WM_TIMER → Window_OnHideTimer               │
├──────────────────────────────────────────────┤
│      UI 层            │    数据层             │
│  ┌──────────────┐    │  ┌────────────────┐   │
│  │ titlebar     │    │  │ storage        │   │
│  │  · 黄底自绘   │    │  │  · AppData 路径│   │
│  │  · 3 按钮    │    │  │  · settings.json│   │
│  │  · 悬停状态  │    │  └────────────────┘   │
│  ├──────────────┤    │  ┌────────────────┐   │
│  │ window       │    │  │ notefile       │   │
│  │  · resize    │    │  │  · 文件读写    │   │
│  │  · 靠边隐藏  │    │  │  · UTF-8 BOM  │   │
│  │  · 自动弹出  │    │  │  · 命名管理   │   │
│  └──────────────┘    │  └────────────────┘   │
│  ┌──────────────┐    │  ┌────────────────┐   │
│  │ 编辑器（待重构）│   │  │ md_parser     │   │
│  │ 目标：基于 D2D │   │  │  · md4c 封装  │   │
│  │  · 单窗口 WYSI│   │  │  · @ ! # 标签 │   │
│  │  · 增量 AST   │   │  └────────────────┘   │
│  └──────────────┘    │                        │
└──────────────────────────────────────────────┘
```

### 模块说明

#### UI 层

| 模块 | 文件 | 功能 | 状态 |
|------|------|------|------|
| 主窗口 | `src/main.c` | WinMain + WndProc + 消息循环 | ✅ 完成 |
| 标题栏 | `src/ui/titlebar.c/.h` | 30px 黄色自绘标题栏，关闭/置顶/隐藏三按钮 | ⚠️ 基于 GDI，后续随渲染层一起重构 |
| 窗口框架 | `src/ui/window.c/.h` | 右下角 resize、靠边隐藏、定时器弹出 | ✅ 完成（与渲染无关，保留）|
| 构件库 | `lib/widget.h/widget.c` | Widget 结构体 + 绘制/命中检测 | ❌ 已废弃（随 GDI 一起移除）|
| 编辑器 | — | 单窗口 WYSIWYG 正文区域 | ❌ 待基于 Direct2D 构建 |

#### 数据层

| 模块 | 文件 | 功能 | 状态 |
|------|------|------|------|
| 存储管理 | `src/storage.c/.h` | AppData 目录初始化、settings.json 读写 | ✅ 完成 |
| 文件操作 | `src/notefile.c/.h` | `.md` 文件的 UTF-8 读写、命名规则 `dn_YYYYMMDD_###` | ✅ 完成（UI 未接入）|
| Markdown 解析 | `src/md_parser.c/.h` | md4c 回调封装，解析 span + @ ! # 标签 | ✅ 完成（UI 未接入）|

### 消息流

```
用户操作 → Windows 消息队列 → GetMessage → DispatchMessage → WndProc
                                                                  │
                        ┌─────────────────────────────────────────┤
                        ↓                                         ↓
                   UI 事件（鼠标/键盘）                     系统事件（绘制/定时器）
                        │                                         │
                        ↓                                         ↓
              Titlebar_HitTest / Window_Resize     WM_PAINT → Titlebar_Draw
              WM_LBUTTONDOWN → 按钮/拖拽           WM_TIMER → Window_OnHideTimer
              WM_MOUSELEAVE → 自动隐藏
```

### 存储布局

```
%APPDATA%\DeskNote\
├── settings.json        # 配置（last_file 等）
└── notes\
    └── dn_YYYYMMDD_###.md   # 便签文件（UTF-8 BOM）
```

### 当时的阻塞问题（现多已解决或过时）

1. **`lib/widget.h` / `lib/widget.c` 已废弃** — `src/ui/titlebar.c` 引用了 Widget 结构体，但 GDI 已移除，该库不再需要。titlebar 需随渲染层一起重构为 Direct2D 绘制
2. **无编辑器** — 单窗口 WYSIWYG 方案已确定，基于 Direct2D + DirectWrite 构建，待实现
3. **数据层未接入 UI** — `notefile.c` 和 `md_parser.c` 的实现代码完整，但待编辑器就绪后接入

---

## 第二部分：当前重构约定（v0.1.x）

### 当前重构阶段的边界约定（MVP 优先）

> 下面这组边界用于指导**当前这一轮重构**，目标不是一步到位实现远期理想架构，
> 而是先稳定落地 `v0.1`：**单窗口、单便签、能输入、能保存、重启能恢复**。

#### 当前目录语义

为减少术语混乱，当前重构阶段统一采用下面的目录含义：

- `app/`：组装层
- `platform/`：平台适配层
- `render/`：渲染层
- `ui/`：界面组件层
- `editor/`：编辑核心
- `core/`：纯数据与语义
- `storage/`：持久化

> 文档中旧的 `view/` 概念，在当前阶段统一折叠为 `ui/`；
> 后续文档统一使用 `ui/` 表达界面组件层。

#### 当前渲染技术决策（重要）

当前重构阶段已经明确：

1. **Windows 渲染后端直接采用 `Direct2D + DirectWrite`**
2. **不再新增 GDI 作为新的过渡渲染路径**
3. `render/` 从第一版开始就按“有生命周期的渲染层”设计

因此当前的 `render/` 含义是：

- 当前阶段：基础渲染后端（帧生命周期、矩形、文字、基础样式）
- 后续阶段：逐步承载文本测量、裁剪、样式绘制、局部重绘等渲染能力

但它仍然**不负责**：

- Markdown 语义判断
- 编辑命令和光标移动规则
- 标题栏 / 编辑区 / 状态栏等具体组件业务

一句话约定：

> `ui` 决定画什么，`render` 负责基于 Direct2D / DirectWrite 把它画出来。

### `ui/button` — 通用按钮组件

`src/ui/button.h` / `button.c` 提供可复用的按钮渲染、状态管理和命中测试，供标题栏、设置面板等所有 UI 组件使用。

```c
// 按钮交互状态
ButtonState { NORMAL, HOVER, PRESSED }

// 按钮实例
Button {
    RenderRect rect;          // 按钮区域（相对窗口客户区）
    ButtonState state;        // 当前交互状态
    int is_visible;           // 可见性
    int is_enabled;           // 是否响应鼠标
    RenderColor bg_color;     // NORMAL 态背景
    RenderColor hover_color;  // HOVER 态背景
    RenderColor pressed_color;// PRESSED 态背景
    const wchar_t* label;     // 标签文字（NULL=纯色块）
}

// 接口
Button_Init()         // 初始化按钮
Button_HitTest()      // 命中测试
Button_UpdateState()  // 根据鼠标位置更新 NORMAL/HOVER/PRESSED
Button_Draw()         // 根据 state 选择颜色并绘制
```

**依赖规则**：`ui/button` 只依赖 `render/`，使用 `Render_FillRect` / `Render_DrawText` 完成绘制，不引入新原语。

**不负责**：按钮点击的业务语义（关闭窗口、最小化等由 `app/` 层通过 `AppShellCommand` 分发）。

#### 一句话职责

| 层 | 回答的问题 | 应该负责 | 不应该负责 |
|------|------|------|------|
| `app/` | 程序怎么接起来 | 初始化、依赖注入、生命周期、事件分发 | 具体绘制、具体编辑规则、具体存储细节 |
| `platform/` | 操作系统发生了什么 | Win32 窗口、消息循环、输入消息、定时器、系统 API | 文档模型、编辑命令、Markdown 语义 |
| `render/` | 东西怎么画出来 | Direct2D/DirectWrite 生命周期、render target、画刷、字体、文本与图元绘制 | 知道什么是标题栏、光标、标签、TODO |
| `ui/` | 界面应该显示什么 | 标题栏、编辑区、状态栏、按钮组件、布局、hover/focus、命中区域 | 底层文本编辑规则、磁盘读写 |
| `editor/` | 文档怎么变化 | 插入删除、光标、选区、键盘命令、撤销重做 | 直接调用 Win32、直接操作 D2D |
| `core/` | 数据是什么 | `Document`、Markdown 语义、标签/TODO 数据结构 | 平台 API、窗口、绘图后端 |
| `storage/` | 状态怎么进出磁盘 | note load/save、settings、启动恢复、原子写入 | 组件绘制、窗口消息、输入派发 |

#### Editor 与 UI / Render 的边界

编辑器在概念上分为两半：

1. **Editor Core（`editor/`）**：维护文本、光标、选区、编辑命令
2. **Editor View（`ui/editor_view.*`）**：把编辑器状态翻译成可显示的界面

因此三者分工固定为：

```text
editor/      -> 维护“内容变成什么样”
ui/          -> 决定“内容显示成什么样”
render/      -> 负责“具体怎么画出来”
```

如果一段代码同时包含：

- Win32 消息处理
- 文本插入/删除规则
- Direct2D 绘图调用
- 文件保存逻辑

说明它已经跨越了多个边界，应继续拆分。

#### 依赖规则（当前阶段）

```text
app
 ├── platform
 ├── render
 ├── ui         ← ui/ 内部按命名区分：button=通用，titlebar=产品
 ├── editor
 ├── core
 └── storage

global rules:
  render -> 不依赖任何层（纯 Direct2D/DirectWrite 封装）
  core   -> 不依赖任何平台与渲染层
  ui     -> 通用组件（如 button）只依赖 render/
            产品组件（如 titlebar、editor_view）可依赖 render、ui/通用组件、editor、core
  app    -> 可依赖 ui、platform、editor、core、storage
  editor -> 可依赖 core
  storage-> 可依赖 core
  platform-> 不依赖 editor/core/storage 的业务实现
```

补充约束：

1. `core/` 不 `#include <windows.h>`
2. `render/` 不出现 `titlebar`、`cursor`、`selection`、`tag` 这类业务名词
3. `storage/` 不持有窗口句柄，不直接操作 UI
4. `platform/` 只把系统事件翻译出来，不直接修改 `Document`
5. `app/` 只做组装与调度，不堆积具体业务逻辑
6. `ui/` 中的通用组件（按文件名约定识别，如 `button`）只依赖 `render/`，不依赖 `app/`、`platform/`、`core/`、`editor/`、`storage/` 中的任何一个——这是保证它们可跨项目复用的约束

#### 当前推荐事件流

```text
Win32 消息
-> platform/ 翻译成统一事件
-> app/ 分发
-> editor/ 修改 Document
-> core/ 提供语义数据
-> ui/ 组织界面
-> render/ 绘制到屏幕
-> storage/ 在合适时机持久化
```

这条链路是当前重构阶段的主干。只要主链保持单向，后续替换渲染器、补 Markdown 渲染、接入更复杂的存储方案时，就不容易互相污染。

#### 后续保存能力的职责归属

随着 `v0.1.x` 往后推进，保存相关能力会继续增强，但职责边界保持不变。

| 能力 | 主要归属 | 辅助归属 | 不应放入 |
|------|------|------|------|
| dirty / 脏标记 | `editor/` 或 `app/` | `core/`（如后续文档状态需要下沉） | `platform/` `ui/` `render/` |
| autosave / 自动保存 | `app/` | `storage/` | `platform/` `ui/` |
| atomic save / 原子保存 | `storage/` | `app/`（只负责触发） | `editor/` `ui/` |
| crash recovery / 崩溃恢复 | `storage/` + `app/` | `editor/`（仅装载文本） | `platform/` `render/` |
| multi-file / 多文件切换保存策略 | `app/` + `storage/` | `editor/` | `platform/` `render/` |

补充说明：

1. **dirty / 脏标记**
   - 本质是“文档是否自上次成功保存后又发生过修改”
   - 可以由 `editor/` 在编辑操作后更新，或由 `app/` 在编辑结果返回后维护
   - 但它不应变成 Win32 窗口层状态，也不应进入 `render/`

2. **autosave / 自动保存**
   - `app/` 决定何时保存，例如空闲一段时间后保存
   - `storage/` 负责真正写盘
   - `platform/` 只提供定时器或时间事件，不直接实现保存策略

3. **atomic save / 原子保存**
   - 这是纯存储层问题：先写临时文件，再替换正式文件
   - `app/` 只知道“调用一次保存”
   - `editor/` 不应知道临时文件名、替换顺序等细节

4. **crash recovery / 崩溃恢复**
   - `storage/` 负责恢复文件的读写
   - `app/` 负责决定启动时是否加载恢复内容
   - `editor/` 只负责装载恢复出的文本

5. **multi-file / 多文件切换保存策略**
   - `app/` 负责“当前文件是谁、切换前要不要保存”
   - `storage/` 负责“这个文件怎么安全写入和读取”
   - `editor/` 仍然只关心“当前装进来的文档是什么”

一句话约定：

> `editor` 负责知道“内容是否变了”，`app` 负责知道“什么时候该保存”，`storage` 负责知道“如何安全保存与恢复”。

#### 后续便签窗口壳层的职责归属

便签产品的“像不像便签”，主要不在 `editor/`，而在窗口壳层。

后续如果把窗口改成**无系统标题栏 / 无系统边框 / 自绘标题区 / 托盘后台 / 桌面驻留**，职责应固定如下：

| 能力 | 主要归属 | 辅助归属 | 不应放入 |
|------|------|------|------|
| 自绘标题栏布局与按钮语义 | `ui/` | `render/` `app/` | `platform/` `editor/` |
| 无边框窗体样式切换 | `platform/` | `app/` | `editor/` `storage/` |
| 拖拽移动 / 边缘缩放 / hit test | `platform/` | `ui/` `app/` | `editor/` `storage/` |
| 标题栏按钮触发的壳层命令 | `app/` | `ui/` `platform/` | `editor/` `render/` |
| 托盘图标 / 托盘菜单 / 隐藏显示生命周期 | `platform/` | `app/` `storage/` | `editor/` `render/` |
| 常驻最前（floating topmost） | `app/` + `platform/` | `storage/` | `editor/` `render/` |
| 贴边占位（edge-reserved / AppBar） | `platform/` + `app/` | `storage/` `ui/` | `editor/` `render/` |
| 壳层模式持久化 | `storage/` | `app/` | `platform/` `editor/` |

补充约定：

1. **“始终置顶”与“贴边占位”不是同一个开关**
   - 前者只是 Z-order 策略，窗口仍是普通浮动窗体
   - 后者会向系统申请工作区边界，影响其他窗口最大化后的可用区域
   - 这两种模式在 `app` 状态、`platform` 实现、`storage` 持久化上都必须分开建模

2. **`platform/` 负责系统语义，`ui/` 负责产品语义**
   - 例如 `WM_NCHITTEST`、`SetWindowPos`、托盘图标、AppBar 注册属于 `platform/`
   - 标题栏按钮排布、hover/pressed 状态、哪一个按钮代表“常驻桌面”属于 `ui/`

3. **`app/` 负责收束壳层模式切换**
   - 用户点击按钮、托盘菜单命令、启动恢复壳层状态，都先进入 `app`
   - 再由 `app` 协调 `platform` 切换窗体行为、协调 `storage` 落盘


## 扩展点与可选功能添加指南

为保证架构一致性与计划可追踪性，关于“可选”或“未来”功能的添加，必须遵循下列流程与约束：

1. 主计划登记（强制）
    - 所有可选/未来功能必须先在主计划 `.ai/plan.md` 中登记为独立项目，包含：目标、产出、优先级、依赖与估时。
    - 不允许仅在阶段文件或 PR 描述中临时写明未来功能。

2. 以 Phase 纳入实施（强制）
    - 新功能的实际实现必须通过新增 `.ai/phases/phase-*.md` 的方式纳入阶段计划，且该 Phase 文件代表本次必须完成的工作。
    - Phase 文件中不得使用“可选/骨架/临时” 等模糊词汇；Phase 意味着“本次要做并需要验收”。

3. 架构层扩展点说明（必写）
    - 在 `docs/design/architecture.md` 中为每个重要扩展点写明：位置（文件/目录）、对外 API/契约、责任边界（哪些层参与）、以及向后兼容策略。
    - 示例（非客户区模块）：
      - 放置：`src/platform/win32/nonclient.c` / `nonclient.h`
      - 建议 API：`Platform_Nonclient_Init(HWND hwnd)`, `Platform_Nonclient_HandleNCHitTest(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`, `Platform_Nonclient_Shutdown()`。
      - `app` 调用点：`App_Init` 调用初始化，`App_RequestClientRebuild` 调用重建；`ui` 提供 `EditorView_OnClientAreaChanged` 回调。

4. 实施检查清单（每次实现前必须列出）
    - 在 Phase 文件中列出：
      - 事件采集点（platform 消息）
      - 编排/策略持有方（app）
      - UI 需实现的回调或适配点（ui/editor_view）
      - 持久化字段（storage/state_store）
      - 最小验收测试用例（手动/自动）

5. 版本与兼容性约定
    - 扩展点必须定义最小 API 契约与错误语义，不得破坏已冻结接口。
    - 若需要行为变更，应优先通过新增 API 或 feature flag 控制，避免直接修改现有合同。

6. 归档与冻结
    - 扩展功能实现完成并验证后，负责该 Phase 的文件应按标准重命名为 `_done`，并在 PR/变更记录中附上验收报告与自动化测试链接。

遵守以上流程可确保未来能力的渐进加入不会破坏当前分层边界，并为代码审查与回滚提供明确的轨迹。
4. **`editor/` 不感知窗体壳层**
   - 编辑器只知道可用客户区尺寸变化
   - 不直接知道标题栏是否自绘、窗口是否在托盘、是否占据桌面边缘

## 第三部分：远期目标架构

### 整体分层

```
┌──────────────────────────────────────────────────┐
│                    app.c                          │
│           应用组装器，只做一件事：初始化 + 启动     │
├──────────────────────────────────────────────────┤
│                                                    │
│    ┌─────────────┐     ┌────────────────────┐     │
│    │    ui/      │     │     editor/         │     │
│    │  UI 组件树  │────▶│   编辑引擎          │     │
│    │             │     │                     │     │
│    │ titlebar    │     │ 光标 · 选区 · 滚动  │     │
│    │ editor_view │     │ 键盘映射 · 撤消     │     │
│    │ 状态栏      │     └────────┬────────────┘     │
│    └──────┬──────┘              │                   │
│           │                     │                   │
│           ▼                     ▼                   │
│    ┌──────────────────────────────────────┐        │
│    │              core/                    │        │
│    │        纯 C 逻辑，零平台依赖          │        │
│    │                                      │        │
│    │ Document  ·  AST  ·  Command · Parser │        │
│    └──────────────────────────────────────┘        │
│                                                    │
│    ┌──────────────────────────────────────┐        │
│    │            platform/                 │        │
│    │   Win32 入口 / 窗口 / 定时器 / 系统   │        │
│    └──────────────────────────────────────┘        │
│    ┌──────────────────────────────────────┐        │
│    │             render/                  │        │
│    │  Direct2D + DirectWrite 渲染能力层    │        │
│    └──────────────────────────────────────┘        │
│                                                    │
└────────────────────────────────────────────────────┘

依赖方向：
    app → ui/editor → core
    app → platform
    ui  → render
    render 在当前阶段直接封装 Windows Direct 技术
```

### 模块间依赖关系

```
                   ┌──────────┐
                   │  app.c   │
                   └────┬─────┘
                        │
              ┌──────────┼───────────────┬──────────┐
              ▼          ▼               ▼          ▼
           ┌──────┐  ┌────────┐      ┌────────┐  ┌──────────┐
           │ ui/  │  │editor/ │      │render/ │  │platform/ │
           │组件树 │  │编辑引擎│      │渲染层   │  │Win32 适配 │
           └──┬───┘  └───┬────┘      └────────┘  └──────────┘
              │          │
              ├──────────┘
              ▼
         ┌──────────┐
         │ core/    │
         │ 纯 C 逻辑 │
         └──────────┘


依赖规则：
    ui/     → 知道 core/、editor/、render/ 存在
    editor/ → 知道 core/ 存在
    editor/ → 不知道 ui/、render/ 和 platform/ 存在
    render/ → 不知道 editor/、core/ 的业务语义
    platform/ → 不知道 editor/、core/、storage/ 的业务实现
    core/   → 不知道任何其他模块存在（纯 C 标准库）
    app.c   → 知道所有模块存在（组装器）
```

### 各层职责

#### core/ — 纯 C 逻辑，零平台依赖

不 include 任何平台头文件（无 windows.h），纯 C 标准库，可以拿到任何平台编译。

| 模块 | 职责 |
|------|------|
| Document | 文档模型：文本内容 + AST 树 |
| AST | 增量更新接口：按键触发节点变更 |
| Command | 命令定义：保存/插入/删除/切换 |
| Parser | 全量解析（md4c 封装），启动时加载用 |

#### editor/ — 编辑引擎，只依赖 core/

不 include 任何平台头文件，不关心"怎么画"，不关心"鼠标从哪里来"。

| 模块 | 职责 |
|------|------|
| 光标 | 位置跟踪、移动、闪烁状态 |
| 选区 | 选择起止、拖选逻辑 |
| 滚动 | 偏移量计算、自动跟随光标 |
| 键盘映射 | 键码 → 命令转换 |
| 撤消 | 操作栈、回退/重做 |

#### ui/ — UI 组件树，组织界面显示逻辑

可以知道 `render/` 存在，但不直接管理 Direct2D / DirectWrite 生命周期；
它负责决定“当前该显示什么”，再调用 `render/` 提供的基础绘制能力。

```
UI 树挂载关系：

root_view
├── titlebar_view          (0, 0, W, 30)
│   ├── close_btn          (右1)
│   ├── pin_btn            (右2)
│   └── hide_btn           (右3)
└── editor_view            (0, 30, W, H-30)
```

每个 UI 组件只画自己的矩形区域，只处理自己区域内的鼠标事件。新增组件只需在 `ui/` 下加新文件，挂到树上。

#### platform/ — 平台适配层

当前阶段的 `platform/` 只先服务 Windows：

```text
platform/win32/entry.c   -> WinMain 入口
platform/win32/window.c  -> 窗口类注册、创建、消息循环、WndProc

后续再逐步扩展：
platform/win32/timer.c
platform/win32/nonclient.c
platform/win32/tray.c
platform/win32/appbar.c
platform/linux/*
```

平台层只处理“操作系统发生了什么”，不承担界面业务和编辑规则。

#### render/ — Direct2D + DirectWrite 渲染层

`render/` 当前直接依赖 Windows Direct 技术，但仍作为独立模块存在，原因是：

1. 把绘制资源生命周期从 `WndProc` 里拿出来
2. 让 `ui/` 不直接操作底层 COM 对象
3. 为后续文本测量、裁剪、富文本样式绘制保留统一入口

当前阶段 `render/` 至少负责：

- 初始化 / 释放渲染资源
- resize 后重建或调整 render target
- begin frame / end frame
- clear / fill rect / draw text

#### storage/ — 持久化层

当前阶段暂不接入显示链，但仍保留为独立层，用于后续：

- settings
- note load/save
- 上次文件恢复
- 原子写入

#### app.c — 组装器

唯一知道所有模块存在的文件，负责启动时初始化各层，收消息后分发给 `ui/`。

**约束：app 层不直接调用系统或平台 API。**
- 系统 API（ShowWindow、SetWindowPos、Shell_NotifyIcon 等）由 `platform/` 层封装
- app 层只通过平台层提供的接口或已有的收束机制（如窗口过程的 switch 分支）与系统交互
- 违反此约束的情形：命令收束 switch 放在 `window.c`（平台层）中，app 层只提交命令不做执行

```
行为：
    Window_Create → Render_Init → Editor_Init → UI_Init → Window_Loop

消息分发：
    消息循环在 platform/window_*.c 内部
    → 翻译成统一 Event 结构体 → 调 app.c 注册的回调
    → App_OnEvent(Event*) → UI_Dispatch 丢给组件树
    → 命中检测找到对应组件
```

### 当前主消息流

```
用户操作
    │
    ▼
platform/window（消息循环 GetMessage → DispatchMessage）
    │ WM_* 在平台层内部消化
    ▼
platform/window_win32.c: WndProc 翻译成统一 Event 结构体
    │ Event{type, x, y, key, ...}
    ▼
app.c: App_OnEvent(Event*) → 全部丢给 ui/ 组件树
    │
    ▼
ui/ 组件树命中检测 → 找鼠标位置落在哪个组件上
    │
    ├── 按钮 → 触发 Command → core/ 执行 → 重绘
    ├── 编辑器点击 → editor/ 定位光标 → 重绘
    └── 壳层操作 → app 收束模式切换 → platform 更新窗口形态 / 托盘 / AppBar
    │
    ▼
editor/core 处理完毕 → Document/AST 变更
    │
    ▼
ui/ 遍历组件树 → EditorView_Draw / Titlebar_Draw
    │
    ▼
render/（Direct2D / DirectWrite 绘制到屏幕）
```

---

### 编辑器核心设计：增量解析 + 事件驱动

#### 设计理念

编辑器不是一个被动的文本容器，而是一个**实时响应键入的脚本处理与渲染平台**。
每一次按键都被分类处理，只有涉及特殊语法的键击才触发 AST 变更。

#### 键入分类

```
用户按键
    │
    ▼
┌─────────────────────────────────────┐
│         按键分类器                   │
│  判断：这是什么类型的输入？           │
└──────┬──────────┬──────────┬────────┘
       │          │          │
       ▼          ▼          ▼
  普通文本    特殊字符     控制键
  (字母/数字/  (# * ` @   (Enter/Backspace/
   标点/空格)   ! [ ] -)   Tab/Delete)
       │          │          │
       │          ▼          ▼
       │    ┌──────────────────────┐
       │    │  特殊语法处理器       │
       │    │  · # + 空格 → 标题行 │
       │    │  · Enter → 新行/待选 │
       │    │  · ` → 代码块开关   │
       │    │  · - [ ] → 勾选框   │
       │    │  · @ → 时间标签     │
       │    │  · ! → 优先级标签   │
       │    │  · #(无空格) → 分类  │
       │    └──────────┬───────────┘
       │               │
       ▼               ▼
┌─────────────────────────────────────┐
│        AST 增量更新                  │
│  只更新受影响的节点，不重建整棵树     │
│  · 新增行 → 插入 Block 节点         │
│  · 标题标记 → 转换 Block 类型       │
│  · 标签输入 → 添加 Span/Attr 节点   │
│  · 删除 → 移除/合并节点             │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│        局部渲染更新                  │
│  只重绘 AST 中变化的部分             │
└─────────────────────────────────────┘
```

#### 上下文状态机

```
状态：普通段落 / 标题行(H1~H6) / 列表项 / 代码块 / 勾选框行

特殊键触发状态转换：
  [Enter] 在普通段落     → 新普通段落
  [Enter] 在列表项       → 新列表项（连续 Enter → 退出列表）
  [#][空格] 在行首      → 标题行 H1
  [##][空格] 在行首     → 标题行 H2
  [-][空格] 在行首      → 列表项
  [-][ ][ ][ ] 在行首   → 勾选框行（未完成）
  [`][`][`][Enter] 在行首 → 代码块
```

#### 与远期数据管线的配合

增量解析引擎替代全量 md4c 解析作为主路径，全量解析保留用于：
- 启动时从 .md 文件加载初始 AST
- 从 journal replay 重建 AST
- 定期一致性校验

#### 这种设计的优势

1. **极致响应**：每次按键只处理增量变化
2. **即时 AST**：编辑器每时刻都知道文档结构
3. **精准渲染**：只更新变化的部分
4. **语义输入**：编辑器根据上下文辅助输入

---

### 数据管线（远期）

```
用户键盘输入
    │
    ▼
┌──────────────────────────────────────────┐
│          1. Working Buffer               │
│  内存中的 Markdown 原始文本（UTF-16）      │
└────────────────────┬─────────────────────┘
                     │ 内容变化触发
                     ▼
┌──────────────────────────────────────────┐
│       2. 快照 + 版本号 (Snapshot)          │
│  取当前 buffer 快照，version=n             │
└────────────────────┬─────────────────────┘
                     │ parseAsync（后台线程）
                     ▼
┌──────────────────────────────────────────┐
│       3. DocumentModel / AST             │
│  md4c 解析输出 → 结构化节点树             │
│  支持 diff 比较 → 生成最小 patch          │
└──────┬───────────────────────┬───────────┘
       │                       │
       ▼                       ▼
┌──────────────┐   ┌────────────────────────┐
│ 4. journal   │   │ 5. patch → 渲染层      │
│ 追加写入操作  │   │ 最小变更更新预览        │
│ 事件到日志    │   │ 避免全量重绘           │
│ 崩溃恢复用   │   │                        │
└──────┬───────┘   └────────────────────────┘
       │ 定期 checkpoint
       ▼
┌──────────────────────────────────────────┐
│       6. note.md（磁盘持久化）             │
│ 合并 buffer → 写入 tmp → rename          │
│ 截断已合并的 journal 事件（compaction）    │
└──────────────────────────────────────────┘
```

管线中的四个核心概念：
1. **Working Buffer** — 内存中的 Markdown 原文本
2. **DocumentModel / AST** — md4c 解析后的结构化节点树
3. **Journal Log** — 追加式操作日志，崩溃后 replay 恢复
4. **Snapshot + Patch** — 异步解析的版本管理和最小渲染更新

---

### 目标数据模型（journal-based）

每个笔记采用文件夹结构：

```
notes/
  <note-id>/
    note.md         # 最近 checkpoint（快速加载）
    state.json      # 元数据：title, tags, last_modified, version
    journal.log     # append-only 日志（用户操作事件序列）
```

三态一致性策略：
- **追加日志**（journal.log）：记录每次操作事件，崩溃恢复
- **内存 buffer**：编辑器工作副本，定期 checkpoint 到 note.md
- **文件索引**（index.json）：全局笔记树结构

---

### 回调模型（远期）

- **结构性输入**（标签、移动、重命名）→ 更新 index.json → UI 刷新 → 写 journal
- **文本输入**（插入/删除/替换）→ append journal + 更新 buffer + 局部渲染

### 原子写入

所有持久化操作：`write tmp → FlushFileBuffers → rename`

### 平台优先级

1. **Windows**（当前）— 首要平台
2. **Linux**（后续）— POSIX 原语适配
3. **macOS**（暂不考虑）

---

### 当前重构迁移路径

```
当前文件                    迁移到
──────────────────────────────────────────────────
src/main.c                → split:
                              src/app/app.c（组装）
                              src/platform/win32/entry.c（WinMain）
                              src/platform/win32/window.c（窗口循环）

src/storage.c / .h        → src/storage/settings.c / .h
                            src/storage/note_store.c / .h

src/notefile.c / .h       → src/storage/note_store.c / .h
                            （按职责拆到持久化层）

src/md_parser.c / .h      → src/core/md_parser.c / .h
                            （纯逻辑，挪过去就行）

src/ui/titlebar.c / .h    → src/ui/titlebar.c / .h
                            （保留为 UI 组件，改用 render/ 绘制）

src/ui/window.c / .h      → src/platform/win32/window.c
                            或拆分为平台行为 + UI 行为

lib/widget.h / widget.c   → ❌ 废弃（随 GDI 一起移除）

src/render.c / .h（待创建）→ src/render/render.c / .h
                             （Direct2D / DirectWrite 实现）

src/editor.c / .h（待创建）→ src/editor/editor.c / .h
                             src/core/parser.c / .h（增量 AST 引擎）
```

### 当前跨平台策略

当前阶段只落地 Windows 版本，跨平台不是正在进行的实现目标。

约束如下：

1. `core/` 保持纯 C，避免引入平台耦合
2. `editor/` 不依赖平台 API
3. `ui/` 不直接管理 Direct2D / DirectWrite 对象
4. `render/` 当前允许直接依赖 Windows Direct 技术
5. Linux / macOS 适配延后到 Windows 主链稳定之后

### 功能模块：桌面停靠栏（AppBar / _NET_WM_STRUT）

| 平台 | 实现方案 | 所在文件 |
|------|---------|--------|
| Windows | `SHAppBarMessage(ABM_NEW/SETPOS/REMOVE)` | `platform/dock_win32.c` |
| Linux | `_NET_WM_STRUT_PARTIAL` X11 协议 | `platform/dock_linux.c` |
| macOS | 无标准 API，暂不支持 | — |

#### 行为

```
右键菜单 → "停靠到桌面"
    │
    ├── 选边（靠左 / 靠右 / 靠上 / 靠下）
    │       │
    │       ▼
    │   platform/dock_*.c: 向系统注册保留边缘空间
    │   Windows: SHAppBarMessage(ABM_NEW)
    │   Linux: XChangeProperty(_NET_WM_STRUT_PARTIAL)
    │
    │   效果：
    │   ├── 其他窗口最大化时自动让位，不盖住 DeskNote
    │   ├── 窗口全屏/分屏布局自动腾出空间
    │   └── 系统停靠栏变化时收到通知自动重对齐
    │
    │   storage/: 保存停靠偏好到 settings.json
    │   ui/titlebar: 停靠模式下隐藏 resize / 隐藏按钮
    │   ui/: 停靠期间固定宽度/高度
    │
    └── 取消停靠
            │
            ▼
        platform/dock_*.c: 释放桌面空间
        Windows: SHAppBarMessage(ABM_REMOVE)
        恢复 resize / 靠边隐藏
```

> **与靠边隐藏的区别：** 靠边隐藏是窗口移出屏幕留 3px 色条（鼠标靠近弹出），
> 停靠是向系统注册保留桌面边缘空间（其他窗口最大化主动让位）。
> 两者是独立功能，可以同时存在但不会同时生效。

#### 与现有模块的关系

| 模块 | 变更 |
|------|------|
| `platform/win32/dock.h` | 新增 `Dock_Register` / `Dock_Unregister` / `Dock_GetWorkArea` 声明 |
| `core/command.c` | 新增 `Cmd_ToggleDock` / `Cmd_SetDockEdge` |
| `storage/settings.c` | 新增 `dock_edge` / `dock_enabled` 配置项 |
| `ui/titlebar` | 停靠模式下禁用 resize 手柄 |
| `ui/` | 停靠模式下宽度/高度固定 |

#### 跨平台策略

统一声明在 `platform/*/dock.h`，各平台 `.c` 链接期选择。macOS 暂为空实现。

```cmake
if(WIN32)
    target_sources(desknote PRIVATE platform/dock_win32.c)
elseif(LINUX)
    target_sources(desknote PRIVATE platform/dock_linux.c)
endif()
```

| | Windows | Linux | macOS |
|--|---------|-------|-------|
| 桌面空间保留 | ✅ 系统级（AppBar API） | ✅ 窗口管理器级（EWMH） | ❌ 无 API |
| 最大化窗口让位 | ✅ 自动 | ✅ 自动 | — |
| 实现工作量 | ~10 行 | ~20 行 X11 属性 | — |
```

### 目标文件结构（按当前重构方向整理）

```
desknote/
├── CMakeLists.txt
├── lib/
│   └── md4c/
├── src/
│   ├── app/
│   │   ├── app.c                  # 组装器：初始化 + 消息分发
│   │   └── app.h
│   ├── core/                      # 纯 C 逻辑，零平台依赖
│   │   ├── document.c / .h        # 文档模型（文本 + AST）
│   │   ├── parser.c / .h          # 增量 AST 引擎
│   │   ├── command.c / .h         # 命令定义
│   │   └── md_parser.c / .h       # md4c 全量解析
│   ├── editor/                    # 编辑引擎
│   │   └── editor.c / .h          # 光标 · 选区 · 滚动 · 键盘映射
│   ├── render/
│   │   ├── render.c / .h          # Direct2D / DirectWrite 渲染层
│   ├── storage/
│   │   ├── settings.c / .h        # 配置读写
│   │   └── note_store.c / .h      # 便签文件读写
│   ├── ui/                        # UI 组件树
│   │   ├── titlebar.c / .h        # 标题栏组件
│   │   ├── editor_view.c / .h     # 编辑区组件
│   │   └── status_view.c / .h     # 状态栏组件
│   └── platform/
│       └── win32/
│           ├── entry.c            # WinMain
│           ├── window.c / .h      # Win32 窗口实现
│           ├── timer.c / .h       # 定时器（后续）
│           └── dock.c / .h        # AppBar 停靠（后续）
├── resources/
└── docs/
```

### 阶段实现顺序

| 阶段 | 做的事 | 产出 |
|------|--------|------|
| **A** | 创建 `render/render.h` + `render.c`，先跑通 `platform -> app -> ui -> render` | 显示链路就绪 |
| **B** | 创建 `editor/` + `ui/editor_view`，实现文字显示 + 光标 + 键盘 | 能打字、能存盘 |
| **C** | 创建 `core/parser.c` 增量 AST 引擎，接入 editor 渲染 | 实时 Markdown 标识 |
| **D** | 右键菜单 → 停靠功能（SHAppBarMessage 注册） | 桌面停靠栏 |
| **H** | 加 `platform/*_linux.c` | 跨平台就绪 |

---

## 架构决策记录

| 决策 | 状态 | 说明 |
|------|------|------|
| ADR-0001 | ⏳ 待创建 | Journal + checkpoint 恢复策略 |
| 渲染技术 | ✅ 确定 | Windows 当前直接使用 Direct2D + DirectWrite，不再新增 GDI 过渡路径 |
| 平台抽象 | ✅ 确定 | 同一函数名各平台各一份 .c，CMake 链接期选择，零运行时开销 |
| 编辑器方案 | ✅ 确定 | 基于 Direct2D/DirectWrite 的单窗口 WYSIWYG，编辑器与渲染整合 |
| 模块分层 | ✅ 确定 | app → ui/editor → core，app → platform，ui → render |
| 桌面停靠 | ✅ 确定 | Windows AppBar / Linux _NET_WM_STRUT，与靠边隐藏独立 |
