# DeskNote 架构设计文档

本文档分两部分：
1. **当前架构**（v0.1.0）—— 实际已实现的代码结构
2. **目标架构**（远期）—— 模块化设计方向

---

## 第一部分：当前架构（v0.1.0）

### 技术栈

| 维度 | 内容 |
|------|------|
| 语言 | C99 |
| UI 层 | Win32 API（GDI 已移除，后续重构为 Direct2D + DirectWrite）|
| Markdown 解析 | md4c（第三方 C 库，submodule）|
| 存储 | `%APPDATA%\DeskNote\` 目录 + JSON 配置 |
| 构建 | CMake 3.20+ + MinGW-w64 GCC |
| 产物 | 单文件 `desknote.exe`，绿色免安装 |

### 架构分层（当前实际）

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

### 当前阻塞问题

1. **`lib/widget.h` / `lib/widget.c` 已废弃** — `src/ui/titlebar.c` 引用了 Widget 结构体，但 GDI 已移除，该库不再需要。titlebar 需随渲染层一起重构为 Direct2D 绘制
2. **无编辑器** — 单窗口 WYSIWYG 方案已确定，基于 Direct2D + DirectWrite 构建，待实现
3. **数据层未接入 UI** — `notefile.c` 和 `md_parser.c` 的实现代码完整，但待编辑器就绪后接入

---

## 第二部分：目标架构（远期）

### 整体分层

```
┌──────────────────────────────────────────────────┐
│                    app.c                          │
│           应用组装器，只做一件事：初始化 + 启动     │
├──────────────────────────────────────────────────┤
│                                                    │
│    ┌─────────────┐     ┌────────────────────┐     │
│    │   view/     │     │     editor/         │     │
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
│    │     各平台实现各一份，CMake 链接期选择 │        │
│    │                                      │        │
│    │ render.h   window.h   timer.h   file.h│       │
│    │ render_win32.c  render_linux.c       │        │
│    │ window_win32.c   window_linux.c      │        │
│    └──────────────────────────────────────┘        │
│                                                    │
└────────────────────────────────────────────────────┘

依赖方向：app → view/editor → core（从左到右依赖）
                         ↓
            platform/ 被 app 和 view 通过抽象接口调用
```

### 模块间依赖关系

```
                    ┌──────────┐
                    │  app.c   │
                    └────┬─────┘
                         │
              ┌──────────┼──────────┐
              ▼          ▼          ▼
         ┌────────┐ ┌────────┐ ┌─────────────┐
         │ view/  │ │editor/ │ │ platform/   │
         │        │ │        │ │ 统一头文件   │
         │组件树   │ │编辑引擎│ │ (.h 声明)   │
         └───┬────┘ └───┬────┘ └──────┬──────┘
             │          │             │
             │          ▼             │
             │    ┌──────────┐        │
             └───▶│ core/    │        │
                  │          │        │
                  │纯 C 逻辑 │        │
                  └──────────┘        │
                                      │
           ┌──────────────────────────┘
           ▼
     ┌─────────────────────┐
     │ platform/*_win32.c  │
     │ platform/*_linux.c  │
     │ 各平台具体实现       │
     │ (app 初始化时调用，  │
     │ view 通过 render.h  │
     │ 抽象接口间接使用)    │
     └─────────────────────┘


依赖规则：
    view/   → 知道 core/ 和 editor/ 存在
    view/   → 不知道 platform/*.c 存在（只通过抽象接口 render.h 画图）
    editor/ → 知道 core/ 存在
    editor/ → 不知道 view/ 和 platform/ 存在
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

#### view/ — UI 组件树，只通过抽象接口画图

不 include 平台头文件，不直接调 Direct2D 或 Cairo，只通过 `platform/render.h` 定义的抽象接口绘制。

```
View 树挂载关系：

root_view
├── titlebar_view          (0, 0, W, 30)
│   ├── close_btn          (右1)
│   ├── pin_btn            (右2)
│   └── hide_btn           (右3)
└── editor_view            (0, 30, W, H-30)
```

每个 View 只画自己的矩形区域，只处理自己区域内的鼠标事件。新增 UI 组件只需在 view/ 下加新文件，挂到树上。

#### platform/ — 各平台实现各一份

统一头文件（`.h`）放在 `platform/`，各平台的具体 `.c` 通过 CMake 链接期选择编译哪份。

```
统一声明（不包含任何平台代码）              Windows 实现              Linux 实现
───────────────                      ─────────────           ───────────
platform/render.h                   platform/render_win32.c  platform/render_linux.c
platform/window.h                   platform/window_win32.c  platform/window_linux.c
platform/timer.h                    platform/timer_win32.c   platform/timer_linux.c
platform/file.h                     platform/file_win32.c    platform/file_linux.c
```

调用方只 include `.h`，不 include 具体的平台 `.c`。运行时就是一条 call 指令，没有 if、没有函数指针、没有虚表。

#### app.c — 组装器

唯一知道所有模块存在的文件，负责启动时初始化各层，收消息后分发给 view 树。

```
行为：
    Window_Create → Render_Init → Editor_Init → View_Init → Window_Loop

消息分发：
    消息循环在 platform/window_*.c 内部
    → 翻译成统一 Event 结构体 → 调 app.c 注册的回调
    → App_OnEvent(Event*) → View_Dispatch 丢给组件树
    → 命中检测找到对应 View
```

### 消息流

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
app.c: App_OnEvent(Event*) → 全部丢给 view/ 组件树
    │
    ▼
view/ 组件树命中检测 → 找鼠标位置落在哪个 View 上
    │
    ├── 按钮 → 触发 Command → core/ 执行 → 重绘
    ├── 编辑器点击 → editor/ 定位光标 → 重绘
    └── 标题栏操作 → 切换置顶/隐藏 → 重绘
    │
    ▼
core/ 处理完毕 → Document/AST 变更
    │
    ▼
view/ 遍历 View 树 → View.Draw(RenderContext)
    │
    ▼
platform/render_*（Direct2D / Cairo 绘制到屏幕）
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

### 现有代码迁移路径

```
当前文件                    迁移到
──────────────────────────────────────────────────
src/main.c                → split:
                              src/app.c（组装）
                              src/platform/window_win32.c（窗口循环）

src/storage.c / .h        → src/core/storage.c / .h
                            （纯逻辑，挪过去就行）

src/notefile.c / .h       → src/core/notefile.c / .h
                            （纯逻辑，挪过去就行）

src/md_parser.c / .h      → src/core/md_parser.c / .h
                            （纯逻辑，挪过去就行）

src/ui/titlebar.c / .h    → src/view/titlebar_view.c / .h
                            （改为 View 组件，画图走 render.h 抽象接口）

src/ui/window.c / .h      → 看情况保留或并入 view/

lib/widget.h / widget.c   → ❌ 废弃（随 GDI 一起移除）

src/render.c / .h（待创建）→ src/platform/render.h（统一声明）
                             src/platform/render_win32.c（D2D 实现）

src/editor.c / .h（待创建）→ src/editor/editor.c / .h
                             src/core/parser.c / .h（增量 AST 引擎）
```

### 跨平台隔离边界

```
                     ┌──────────────────────────┐
                     │  跨平台可编译的模块       │
                     │                          │
                     │  core/  纯 C，零平台依赖  │
                     │  editor/ 不 include 平台  │
                     │  view/   只依赖抽象接口    │
                     │  platform/*.h 纯声明      │
                     └──────────────────────────┘
                                    │
                        链接期 CMake 选择
                                    │
                     ┌──────────────┴──────────────┐
                     ▼                              ▼
         ┌────────────────────┐        ┌────────────────────┐
         │ Windows 平台实现    │        │ Linux 平台实现      │
         │ window_win32.c     │        │ window_linux.c     │
         │ render_win32.c     │        │ render_linux.c     │
         │ (Direct2D)         │        │ (Cairo/X11)        │
         │ timer_win32.c      │        │ timer_linux.c      │
         │ file_win32.c       │        │ file_linux.c       │
         └────────────────────┘        └────────────────────┘

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
    │   core/storage: 保存停靠偏好到 settings.json
    │   view/titlebar_view: 停靠模式下隐藏 resize / 隐藏按钮
    │   view/: 停靠期间固定宽度/高度
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
| `platform/dock.h` | 新增 `Dock_Register` / `Dock_Unregister` / `Dock_GetWorkArea` 声明 |
| `core/command.c` | 新增 `Cmd_ToggleDock` / `Cmd_SetDockEdge` |
| `core/storage.c` | 新增 `dock_edge` / `dock_enabled` 配置项 |
| `view/titlebar_view` | 停靠模式下禁用 resize 手柄 |
| `view/` | 停靠模式下宽度/高度固定 |

#### 跨平台策略

统一声明在 `platform/dock.h`，各平台 `.c` 链接期选择。macOS 暂为空实现。

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

### 目标文件结构（远期）

```
desknote/
├── CMakeLists.txt
├── lib/
│   └── md4c/
├── src/
│   ├── app.c                      # 组装器：初始化 + 消息分发
│   ├── core/                      # 纯 C 逻辑，零平台依赖
│   │   ├── document.c / .h        # 文档模型（文本 + AST）
│   │   ├── parser.c / .h          # 增量 AST 引擎
│   │   ├── command.c / .h         # 命令定义
│   │   ├── storage.c / .h         # 存储路径 & 配置
│   │   ├── notefile.c / .h        # 便签文件读写
│   │   └── md_parser.c / .h       # md4c 全量解析
│   ├── editor/                    # 编辑引擎
│   │   └── editor.c / .h          # 光标 · 选区 · 滚动 · 键盘映射
│   ├── view/                      # UI 组件树
│   │   ├── view.h                 # View 基类定义
│   │   ├── titlebar_view.c / .h   # 标题栏组件
│   │   ├── editor_view.c / .h     # 编辑区组件
│   │   └── status_view.c / .h     # 状态栏组件
│   └── platform/                  # 各平台实现各一份
│       ├── render.h               # 渲染统一声明
│       ├── render_win32.c         # Direct2D 实现
│       ├── window.h               # 窗口统一声明
│       ├── window_win32.c         # Win32 窗口实现
│       ├── timer.h                # 定时器统一声明
│       ├── timer_win32.c          # Win32 定时器实现
│       ├── file.h                 # 文件 I/O 统一声明
│       ├── file_win32.c           # Win32 文件 I/O 实现
│       ├── dock.h                 # 桌面停靠统一声明
│       └── dock_win32.c           # Win32 AppBar 停靠实现
├── resources/
└── docs/
```

### 阶段实现顺序

| 阶段 | 做的事 | 产出 |
|------|--------|------|
| **A** | 创建 `platform/render.h` + `render_win32.c`，重构 titlebar 到 D2D | 渲染基础就绪 |
| **B** | 创建 `editor/` + `view/editor_view`，实现文字显示 + 光标 + 键盘 | 能打字、能存盘 |
| **C** | 创建 `core/parser.c` 增量 AST 引擎，接入 editor 渲染 | 实时 Markdown 标识 |
| **D** | 右键菜单 → 停靠功能（SHAppBarMessage 注册） | 桌面停靠栏 |
| **H** | 加 `platform/*_linux.c` | 跨平台就绪 |

---

## 架构决策记录

| 决策 | 状态 | 说明 |
|------|------|------|
| ADR-0001 | ⏳ 待创建 | Journal + checkpoint 恢复策略 |
| 渲染技术 | ✅ 确定 | Windows: Direct2D + DirectWrite；跨平台: 链接期选择策略 |
| 平台抽象 | ✅ 确定 | 同一函数名各平台各一份 .c，CMake 链接期选择，零运行时开销 |
| 编辑器方案 | ✅ 确定 | 基于 Direct2D 的单窗口 WYSIWYG，编辑器与渲染整合 |
| 模块分层 | ✅ 确定 | app → view/editor → core → platform 单向依赖 |
| 桌面停靠 | ✅ 确定 | Windows AppBar / Linux _NET_WM_STRUT，与靠边隐藏独立 |
