# Plan 进度表（Progress Tracker）

## 当前阶段：A — 搭建渲染基础

| ID | Title | Status | Estimate | Progress | Blockers | Notes |
|---:|:------|:------:|:--------:|:--------:|:--------:|:------|
| A.1 | 创建 `src/render.c` / `render.h`（D2D + DWrite 生命周期） | 📝 todo | 2d | 0% | — | Factory → RenderTarget → DWriteFactory → TextFormat |
| A.2 | 创建 `src/platform/render.h` / `render_win32.c` | 📝 todo | 1d | 0% | — | 统一声明，链接期选择 |
| A.3 | 初始化 `lib/md4c/` 子模块 | ⏳ todo | 0.5d | 0% | 需 git submodule update | |
| A.4 | 重构 titlebar.c 从 GDI 改 D2D | 📝 todo | 1d | 0% | 依赖 A.1 | |
| A.5 | 废弃 lib/widget.h/c | 📝 todo | — | 0% | — | 从 CMakeLists.txt 移除 |
| A.6 | 验证编译通过 | ⏳ todo | 0.5d | 0% | 依赖 A.1~A.5 | |

## 阶段 B：文本编辑

| ID | Title | Status | Estimate | Progress | Blockers | Notes |
|---:|:------|:------:|:--------:|:--------:|:--------:|:------|
| B.1 | 编辑器核心（文本缓冲 + 光标 + 键盘 + D2D 绘制） | 📝 todo | 3d | 0% | 依赖 A.1 | 单窗口，WYSIWYG |
| B.2 | 编辑器 ↔ notefile 联通 | ⏳ todo | 1d | 0% | 依赖 B.1 | |
| B.3 | 失焦自动保存 | ⏳ todo | 0.5d | 0% | 依赖 B.1 | |
| B.4 | 标题栏显示文件名 | ⏳ todo | 0.5d | 0% | 依赖 B.1 | |

## 阶段 C：Markdown 渲染接入

| ID | Title | Status | Estimate | Progress | Blockers | Notes |
|---:|:------|:------:|:--------:|:--------:|:--------:|:------|
| C.1 | 增量 AST 引擎（core/parser.c） | 📝 todo | — | 0% | 你的核心 | 按键分类 + 状态机 + 增量更新 |
| C.2 | AST → 编辑器渲染联通 | ⏳ todo | 2d | 0% | 依赖 C.1, B.1 | |
| C.3 | @ ! # 标签着色 | ⏳ todo | 1d | 0% | 依赖 C.2 | |
| C.4 | 勾选框渲染与交互 | ⏳ todo | 1d | 0% | 依赖 C.2 | |

## ✅ 已完成

| ID | Title | Status | Notes |
|---:|:------|:------:|:------|
| 0.1 | 环境搭建（MSYS2 + MinGW + CMake） | ✅ done | |
| 0.2 | 编译第一个 Hello World | ✅ done | |
| 1.1 | 注册窗口类、创建窗口、消息循环 | ✅ done | |
| 1.2 | WS_POPUP 无边框、自绘黄色标题栏（GDI） | ✅ done | titlebar.c |
| 1.3 | 拖拽移动 | ✅ done | main.c |
| 1.4 | 关闭按钮 | ✅ done | titlebar.c |
| 1.4b | 标题栏拆分为独立文件 | ✅ done | titlebar.h/.c |
| 1.4c | 关闭按钮悬停变红 | ✅ done | titlebar.c |
| 1.4d | 应用图标 | ✅ done | desknote.rc |
| 1.5 | 右下角 resize | ✅ done | window.c |
| 1.6 | 置顶按钮 | ✅ done | titlebar.c |
| 1.7 | 靠边隐藏 | ✅ done | window.c |
| 3.1 | `%APPDATA%\DeskNote\` 目录创建 | ✅ done | storage.c |
| 3.2 | `notes\` 子目录创建 | ✅ done | storage.c |
| 3.7~3.9 | 右键弹出菜单 | ✅ done | main.c |
| 3.16 | 关于弹窗 | ✅ done | main.c |
| 4.1~4.3 | 文件读写、命名规则 | ✅ done | notefile.c（UI 未接入）|
| 4.6 | 关闭时保存 last_file | ✅ done | notefile.c |
| 2.2~2.4 | md4c 集成、md_parser 封装 | ✅ done | md_parser.c |
