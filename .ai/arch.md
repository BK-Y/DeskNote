# DeskNote 架构文档

> 本文档定义 7 层结构、各模块职责边界、接口规范与调用限制。
> 所有计划文件必须先对照本文档检查分层合规性。
> 违反本文档的代码视为架构违规，必须修正后方可合入。

---

## 一、架构总览

### 1.1 分层全景

```
┌─────────────────────────────────────────────────────────────────────┐
│                           入口层                                     │
│              platform/win32/entry.c (WinMain)                       │
├─────────────────────────────────────────────────────────────────────┤
│                              platform                               │
│         窗口管理 / 非客户区 / AppBar / Win32 消息循环                │
│           ┌─────────────┐  ┌──────────────┐  ┌──────────┐          │
│           │  window.c   │  │  nonclient.c  │  │ appbar.c │          │
│           │  (消息循环)  │  │  (自绘边框)   │  │ (AppBar) │          │
│           └─────────────┘  └──────────────┘  └──────────┘          │
├─────────────────────────────────────────────────────────────────────┤
│                               app                                   │
│          应用编排 / 生命周期 / 消息分发 / 窗口事件转发               │
│                          app.c / app.h                              │
├─────────────────────────────────────────────────────────────────────┤
│                              page                                   │
│     页面管理：统一全局配置入口 / 窗口+note+配置的组合生命周期         │
│                     page/ (尚未拆分)                                │
├─────────────────────────────────────────────────────────────────────┤
│                              shell                                  │
│          窗口模式管理 / 模式切换 / 状态持久化                         │
│                       shell/ (尚未拆分)                             │
├─────────────────────────────────────────────────────────────────────┤
│                   ui                      render                    │
│    ┌──────────────┐  ┌─────────────┐  ┌──────────────────┐         │
│    │  editor_view │  │  titlebar   │  │  render.c/h      │         │
│    │  (编辑区绘制) │  │  (标题栏)   │  │  (Direct2D封装)  │         │
│    ├──────────────┤  ├─────────────┤  └──────────────────┘         │
│    │  button.c/h  │  └─────────────┘                                │
│    │  (按钮状态)   │                                                │
│    └──────────────┘                                                 │
├─────────────────────────────────────────────────────────────────────┤
│                             editor                                  │
│               编辑器逻辑 / 光标 / 选区 / 按键处理                    │
│                         editor.c / editor.h                         │
├─────────────────────────────────────────────────────────────────────┤
│                              core                                   │
│                    文档数据模型 / 文本缓冲区                         │
│                        document.c / document.h                      │
├─────────────────────────────────────────────────────────────────────┤
│                            storage                                  │
│             持久化 / state.ini / note.md / 原子写入                  │
│        ┌──────────┐  ┌────────────┐  ┌───────────────┐             │
│        │ state_   │  │ note_store │  │ storage_write │             │
│        │ store    │  │            │  │ (原子写入)    │             │
│        └──────────┘  └────────────┘  └───────────────┘             │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 调用方向

```
platform → app → page
                ├─ shell (窗口模式) → platform/appbar
                ├─ editor → core
                └─ storage
         → ui → render
```

严格单向，禁止反向调用：
- **app 层不得直接调用 render 或 ui 的绘制接口**（通过 page 层分发）
- **page 层是全局配置的唯一入口**——shell/editor/storage 的行为差异通过 page 层拉齐
- **editor 层不得直接调用 platform 层**（按键已由 app 层翻译为 `EditorKey` 枚举）
- **ui 层不得直接调用 storage 层**
- **core 层不依赖任何其他模块**

### 1.3 术语约定

代码注释中统一使用以下标记标注每行代码的性质：

| 标记 | 含义 | 示例 |
|---|---|---|
| `/* target */` | 调用这个函数的核心目的 | `AppBar_Register(hwnd)  /* target */` |
| `/* need */` | 为完成 target 必须做的配套操作 | `StateStore_Save(&g_shell)  /* need */` |

对外沟通（讨论、PR 评审）时，使用标准术语 **side effect**，指向同一个概念。

---

## 二、模块定义

### 2.1 platform/win32

#### window

| 字段 | 内容 |
|------|------|
| **位置** | `src/platform/win32/window.c` / `window.h` |
| **归属层** | platform |
| **职责** | Win32 窗口创建、消息循环、`WindowProc`、消息分发到 app 层回调；`WinMain` 入口 |
| **不负责** | 不处理任何业务逻辑；不直接调 render/ui/editor/storage |
| **对外接口** | `Window_Run()` — 创建窗口并进入消息循环 |
| **调用方限制** | 仅 `app.c` 的 `App_Run()` 可调用 |
| **使用示例** | `int App_Run(void) { return Window_Run(); }` |
| **违规示例** | 在 `WindowProc` 中直接调 `EditorView_Draw` 或 `StateStore_Save` |

#### nonclient

| 字段 | 内容 |
|------|------|
| **位置** | `src/platform/win32/nonclient.c` / `nonclient.h` |
| **归属层** | platform |
| **职责** | 自绘窗口非客户区（标题栏命中检测、缩放热区、边框内边距计算） |
| **不负责** | 不绘制标题栏视觉（属 ui/titlebar）；不处理业务模式切换 |
| **对外接口** | `Platform_Nonclient_Init`、`Platform_Nonclient_SetMetrics`、`Platform_Nonclient_ComputeContentRect`、`Platform_Nonclient_HandleNCHitTest`、`Platform_Nonclient_Shutdown`、`Platform_Nonclient_IsEnabled` |
| **调用方** | `app.c`（Init/ApplyChrome）、`window.c`（WM_NCHITTEST） |

#### appbar

| 字段 | 内容 |
|------|------|
| **位置** | `src/platform/win32/appbar.c` / `appbar.h` |
| **归属层** | platform |
| **职责** | Windows AppBar API 封装（ABM_NEW / ABM_QUERYPOS / ABM_SETPOS / ABM_REMOVE） |
| **不负责** | 不决定何时注册/注销 AppBar（由 shell 层决定）；不管理窗口模式状态 |
| **对外接口** | `AppBar_Register`、`AppBar_Unregister`、`AppBar_SetPosition`、`AppBar_ReRegister`、`AppBar_IsRegistered` |
| **调用方** | 仅 `shell` 层（当前在 `app.c`）和 `window.c`（WM_DISPLAYCHANGE 等系统事件触发的 ReRegister） |
| **禁止调用方** | `editor`、`render`、`ui`、`core`、`storage` |
| **使用示例** | `AppBar_Register(hwnd); AppBar_SetPosition(hwnd, APP_DOCK_RIGHT, 240);` |
| **违规示例** | 在 `editor.c` 或 `titlebar.c` 中调 `AppBar_Register` |

---

### 2.2 app

| 字段 | 内容 |
|------|------|
| **位置** | `src/app/app.c` / `app.h` |
| **归属层** | app |
| **职责** | 应用生命周期（Init/Shutdown/Run）；窗口事件注册与转发（OnResize/OnPaint/OnChar/OnKeyDown 等）；托盘图标管理；命令队列（`App_SubmitShellCommand`） |
| **不负责** | 模式切换的副作用编排（→ shell）；编辑器导航（→ editor）；渲染细节（→ render） |
| **对外接口** | `App_Run`、`App_Init`、`App_Shutdown`、`App_OnResize`、`App_OnPaint`、`App_OnChar`、`App_OnKeyDown`、`App_OnLeftButtonDown`、`App_OnMouseMove`、`App_OnMouseWheel`、`App_OnTick`、`App_SubmitShellCommand`、`App_TakePendingShellCommand`、`App_InitTrayIcon`、`App_DestroyTrayIcon`、`App_HideToTray`、`App_RestoreFromTray`、`App_RequestClientRebuild` |
| **调用方** | `platform/win32`（window.c/entry.c） |
| **禁止调用方** | `ui`、`render`、`editor`、`core`、`storage` 不得调 app 层函数 |
| **使用示例** | 见 `window.c` 中 `WinMain → App_Run()` 和 `WindowProc` 中各 `App_On*` 回调 |
| **当前违规** | `app.c` 中混入了 shell 模式切换逻辑（待拆分到 `shell/`）；混入了编辑器导航逻辑（待迁移到 `editor/`） |

---

### 2.3 page（尚未拆分，位置预留）

> 当前所有状态管理散落在 `app.c` 及各处，以下为拆分后的目标定义。

| 字段 | 内容 |
|------|------|
| **位置** | 目标：`src/page/page.c` / `page.h` |
| **归属层** | app |
| **职责** | 窗口（page）的完整生命周期管理：统一全局配置入口（shell 模式、editor 设置、存储策略等）；将全局配置和 per-page 配置（颜色/位置/大小）路由到正确的持久化位置（state.ini vs note 文件）；多窗口时作为窗口+note+配置的组合容器 |
| **不负责** | 具体模式切换的伴生行为编排（→ shell）；编辑器核心逻辑（→ editor）；Win32 窗口句柄管理（→ platform） |
| **对外接口** | 目标：`Page_Create`、`Page_Close`、`Page_GetConfig`、`Page_SetConfig` |
| **禁止调用方** | `editor`、`ui`、`render`、`core`、`platform` |
| **当前状态** | 待拆分。当前相关逻辑散落在 `app.c` 中（`App_Init` 中的状态恢复、`App_ApplyShellStateData`、`App_WriteShellStateData`） |

---

### 2.4 shell（尚未拆分，位置预留）

> 当前 shell 逻辑分散在 `app.c`、`appbar.c`、`window.c` 中，以下为拆分后的目标定义。

| 字段 | 内容 |
|------|------|
| **位置** | 目标：`src/shell/shell.c` / `shell.h` |
| **归属层** | app |
| **职责** | 窗口模式状态（NONE / FLOATING_TOPMOST / EDGE_RESERVED）的唯一管理；模式切换时的伴生行为编排（AppBar 注册/注销、Z 序修改、状态持久化） |
| **不负责** | 编辑器导航（→ editor）；渲染（→ render）；UI 绘制（→ ui）；Win32 消息循环（→ platform）；AppBar API 的具体 Win32 实现细节（→ platform/appbar） |
| **对外接口** | 目标：`Shell_SetMode(hwnd, mode, ...)`、`Shell_GetMode()` |
| **禁止调用方** | `editor`、`render`、`ui`、`core`、`storage` 禁止调用 shell |
| **当前状态** | 待拆分。当前相关代码散落在 `app.c`（`App_TryRegisterAppBarFromState`、cmd103、`App_OnEndDrag`、`App_HideToTray`、`App_RestoreFromTray`）和 `window.c`（WM_DISPLAYCHANGE / WM_SETTINGCHANGE / ABN_FULLSCREENAPP） |

---

### 2.4 ui

#### editor_view

| 字段 | 内容 |
|------|------|
| **位置** | `src/ui/editor_view.c` / `editor_view.h` |
| **归属层** | ui |
| **职责** | 编辑区布局计算；文本行视觉定位与命中检测；光标/选区绘制 |
| **不负责** | 编辑器逻辑（→ editor）；渲染原语（→ render） |
| **对外接口** | `EditorView_CalculateLayout`、`EditorView_Draw`、`EditorView_GetCursorVisualPosition`、`EditorView_GetTextRect`、`EditorView_HitTestCursor`、`EditorView_OnClientAreaChanged` |
| **调用方** | `app.c`（App_OnPaint 中调 Draw，导航中调 GetCursorVisualPosition/HitTestCursor） |
| **禁止调用方** | `editor`、`core`、`storage`、`platform` |
| **使用示例** | `EditorView_Draw(render, doc, cursor, scroll, sel_start, sel_end);` |

#### titlebar

| 字段 | 内容 |
|------|------|
| **位置** | `src/ui/titlebar.c` / `titlebar.h` |
| **归属层** | ui |
| **职责** | 标题栏视觉绘制（背景、标题文字、模式指示灯、菜单按钮） |
| **不负责** | 按钮交互命中检测（→ app 层 OnLeftButtonDown）；窗口拖拽（→ nonclient） |
| **对外接口** | `Titlebar_Draw`、`Titlebar_UpdateStatus` |
| **调用方** | `app.c`（App_OnPaint 中调） |
| **使用示例** | `Titlebar_Draw(render, hwnd, &g_app.shell, &g_app.menu_button_state);` |

#### button

| 字段 | 内容 |
|------|------|
| **位置** | `src/ui/button.c` / `button.h` |
| **归属层** | ui |
| **职责** | 按钮状态计算（Normal / Hovered / Pressed / Disabled）与渲染 |
| **不负责** | 窗口布局；标题栏之外的绘制 |
| **对外接口** | `Button_UpdateState`、`Button_Draw` |
| **调用方** | `titlebar.c` |

---

### 2.6 render

| 字段 | 内容 |
|------|------|
| **位置** | `src/render/render.c` / `render.h` |
| **归属层** | render |
| **职责** | Direct2D / DirectWrite 封装：渲染目标生命周期、清背景、填充矩形、绘制边框、文字绘制、裁剪 |
| **不负责** | 任何业务语义；不管理字体、样式、主题；不进行命中检测 |
| **对外接口** | `Render_Create`、`Render_Init`、`Render_Destroy`、`Render_Resize`、`Render_BeginFrame`、`Render_EndFrame`、`Render_Clear`、`Render_FillRect`、`Render_DrawRect`、`Render_DrawText`、`Render_CreateTextFormat`、`Render_DestroyTextFormat`、`Render_PushClip`、`Render_PopClip` |
| **调用方** | `app.c`、`ui/editor_view.c`、`ui/titlebar.c`、`ui/button.c` |
| **禁止调用方** | `editor`、`core`、`storage`、`platform` |
| **使用示例** | `Render_BeginFrame(render); Render_Clear(render, bg); Titlebar_Draw(render,...); EditorView_Draw(render,...); Render_EndFrame(render);` |

---

### 2.7 editor

| 字段 | 内容 |
|------|------|
| **位置** | `src/editor/editor.c` / `editor.h` |
| **归属层** | editor |
| **职责** | 编辑状态管理（光标、选区、插入/删除字符、纵向移动导航）；按键到编辑器操作的翻译 |
| **不负责** | 文本渲染（→ ui/editor_view）；文档持久化（→ storage）；窗口管理（→ platform）；导航上下文（当前 `app.c` 中 `App_NavigatorGetVisualPosition` 应移入此模块） |
| **对外接口** | `Editor_Init`、`Editor_Free`、`Editor_SetText`、`Editor_SetCursor`、`Editor_HandleChar`、`Editor_HandleKey`、`Editor_MoveCursorVertical`、`Editor_GetDocument`、`Editor_GetCursor`、`Editor_SetSelection`、`Editor_ClearSelection`、`Editor_GetSelectionAnchor`、`Editor_GetSelectionActive`、`Editor_HasSelection`、`Editor_GetWordBoundary`、`Editor_InsertText` |
| **调用方** | `app.c` |
| **禁止调用方** | `platform`、`render`、`storage`、`core`（document 除外，editor 依赖 document） |
| **使用示例** | `Editor_HandleChar(&g_app.editor, ch); Editor_HandleKey(&g_app.editor, EDITOR_KEY_ENTER);` |
| **当前违规** | `app.c` 中 `App_NavigatorGetVisualPosition`、`App_NavigatorHitTestPoint`、`App_MoveCursorVertical`、`App_EnsureCaretVisible` 涉及编辑器导航逻辑，应迁入 `editor/` |

---

### 2.8 core

| 字段 | 内容 |
|------|------|
| **位置** | `src/core/document.c` / `document.h` |
| **归属层** | core |
| **职责** | 文档数据模型：UTF-16 文本缓冲区的增删改查与生命周期 |
| **不负责** | 光标、选区、样式、渲染、平台消息、持久化格式 |
| **对外接口** | `Document_Init`、`Document_Free`、`Document_Clear`、`Document_SetText`、`Document_GetText`、`Document_GetLength`、`Document_InsertChar`、`Document_DeleteChar` |
| **调用方** | `editor` 层 |
| **禁止调用方** | `platform`、`app`、`ui`、`render`、`storage` |

---

### 2.9 storage

#### state_store

| 字段 | 内容 |
|------|------|
| **位置** | `src/storage/state_store.c` / `state_store.h` |
| **归属层** | storage |
| **职责** | `state.ini` 的读写（UTF-16 LE）、路径管理 |
| **不负责** | 模式切换逻辑（→ shell）；note 文件读写（→ note_store） |
| **对外接口** | `StateStore_GetRootPath`、`StateStore_GetStatePath`、`StateStore_Load`、`StateStore_Save` |
| **调用方** | `app.c`（当前）、`shell`（拆分后） |
| **使用示例** | `StateData state; StateStore_Load(&state); state.shell_resident_mode = MODE_EDGE_RESERVED; StateStore_Save(&state);` |
| **违规示例** | 在 state_store 内判断 mode 并自动触发 AppBar 操作 |

#### note_store

| 字段 | 内容 |
|------|------|
| **位置** | `src/storage/note_store.c` / `note_store.h` |
| **归属层** | storage |
| **职责** | note.md 文件的读写（文件路径、命名、UTF-8 编码） |
| **不负责** | 文档内存模型（→ core）；自动保存策略（→ app） |
| **对外接口** | 见 `note_store.h` |

#### storage_write

| 字段 | 内容 |
|------|------|
| **位置** | `src/storage/storage_write.c` / `storage_write.h` |
| **归属层** | storage |
| **职责** | 原子写入（写入临时文件 → 重命名 → 目标文件覆盖） |
| **不负责** | 文件格式解析、内容组装 |
| **对外接口** | 见 `storage_write.h` |

---

## 三、依赖规则

### 3.1 层间依赖方向

```
platform → app → page
                ├─ shell → platform/appbar
                ├─ editor → core
                └─ storage
         → ui → render
```

- 上层可依赖下层，下层不可依赖上层
- 同层可跨模块调用（如 `ui/titlebar → ui/button`）
- 跨层调用必须经过直接上层（`app` 不可跳过 `editor` 直接调 `core` 的 `Document_InsertChar`——实际通过 `Editor_HandleChar` 间接调用）

### 3.2 禁止的反向调用

```
editor → platform       ✗  editor 不调 window/appbar/nonclient
editor → render         ✗  editor 不调 Render_FillRect
editor → storage        ✗  editor 不写磁盘
render → editor         ✗  render 不处理编辑器逻辑
render → app            ✗  render 不调 App_OnResize
storage → app           ✗  storage 不调 App_OnEndDrag
platform → app (命令)    ✓  platform 调 App_OnResize/OnPaint 等转发
platform → app (模式)    ✗  platform 不直接调 App_SetResidentMode（应通过命令队列）
```

### 3.3 平台差异隔离

平台相关代码（Win32 API 调用）全部封装在 `src/platform/win32/` 内：

```
src/platform/
  └─ win32/       ← 当前只有 Win32 实现
  └─ (未来) linux/
  └─ (未来) macos/
```

其他层通过 `platform/` 的公开头文件与平台交互，不直接引用 Win32 宏或类型（`<windows.h>` 已在各头文件中隔离）。

---

## 四、违反架构的处理

1. 发现违规代码 → 在 `.ai/decisions.md` 中记录违规位置
2. 判断是"需要即时修复"还是"纳入计划修复"
3. 即时修复 → 直接改代码
4. 计划修复 → 在 `.ai/plans/` 中创建设计计划，排入后续阶段
