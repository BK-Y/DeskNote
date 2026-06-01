# DeskNote

DeskNote 是一个正在重构中的 Windows 桌面便签应用。当前主线目标不是完整 Markdown 产品，而是先把一个**严格分层、可持续演进**的单窗口单便签编辑器打稳。

## 当前状态

当前代码已经实现：

- 基于 **Win32 + Direct2D + DirectWrite** 的最小编辑器窗口
- 键盘输入、回车、退格、左右移动
- 精确光标定位，以及系统光标 / 中文 IME 跟随
- 启动加载、关闭保存
- 将运行状态与默认便签保存到 `%LOCALAPPDATA%\DeskNote\`

当前**尚未完成**的主线能力：

- 点击定位光标
- 长文本最小可浏览（垂直滚动 / 光标跟随视口）
- Markdown 语义解析与样式化显示
- 多便签、设置面板、系统托盘等外围功能

## 构建

先初始化子模块：

```bash
git clone --recurse-submodules https://github.com/BK-Y/DeskNote.git
```

Windows 下使用 CMake 构建：

```bash
cmake -S . -B build
cmake --build build
```

构建完成后运行：

```bash
.\build\desknote.exe
```

## 当前源码结构

```text
src/
├── app/             # 组装与生命周期调度
├── core/            # 文本文档等纯数据模型
├── editor/          # 编辑规则与光标状态
├── platform/win32/  # Win32 窗口、消息循环、输入转发
├── render/          # Direct2D + DirectWrite 渲染与测量
├── storage/         # LocalAppData 状态与便签持久化
└── ui/              # editor_view 等界面语义与布局
```

`lib\md4c\` 仍然保留为子模块，但当前主线实现还没有重新接入 Markdown 解析链。

## 当前存储布局

```text
%LOCALAPPDATA%\DeskNote\
├── note.md
└── state.ini
```

- `note.md`：当前默认便签内容
- `state.ini`：运行时状态持久化文件（UTF-16 LE 编码）

## state.ini 配置项参考

文件路径：`%LOCALAPPDATA%\DeskNote\state.ini`
编码：UTF-16 LE（Windows "Unicode"），非此编码程序将拒绝读取

### 字段总表

| 键名 | 类型 | 值范围 | 引入阶段 | 说明 |
|------|------|-------|---------|------|
| `version` | int | 固定 2 | 初始 | 格式版本号 |
| `last_file` | 路径 | — | 初始 | 上次打开的文档路径 |
| `use_custom_chrome` | 0/1 | 固定 1 | 初始 | 自定义标题栏（不可改） |
| `titlebar_height` | int | 默认 32 | 初始 | 标题栏高度（px） |
| `frame_visual_thickness` | int | 默认 1 | 初始 | 边框视觉厚度（px） |
| `shell_resident_mode` | 0-2 | 见下表 | Shell-3c_2 | 窗口常驻模式 |
| `presence_state` | 0-2 | 0=VISIBLE, 1=HIDDEN, 2=EXITING | Shell-3c_2 | 窗口可见状态 |
| `last_floating_*` | int | — | Shell-4a_2 | 上次浮动置顶时的窗口坐标与尺寸 |
| `dock_edge` | 0-3 | 见下表 | Shell-5a | AppBar 贴边方向 |
| `dock_thickness` | int | 200-500（运行时 clamp） | Shell-5a | 贴边保留区厚度（px） |
| `release_on_hide_mode` | 0-2 | 0=never, 1=remember, 2=clear | repair-5-a-4 | 隐藏时释放策略（默认 1） |
| `release_on_drag_mode` | 0-2 | 0=never, 1=to_topmost, 2=to_none | repair-5-a-4 | 拖动时释放策略（默认 1） |

### shell_resident_mode（常驻模式）

| 值 | 含义 | 行为 |
|----|------|------|
| 0 | NONE | 普通窗口 |
| 1 | FLOATING_TOPMOST | 浮动置顶 |
| 2 | EDGE_RESERVED | 注册为 AppBar 贴边，启动/托盘恢复时自动注册 |

### dock_edge（贴边方向）

| 值 | 对应 ABE_ | 贴边位置 |
|----|-----------|---------|
| 0 | ABE_LEFT | 左 |
| 1 | ABE_TOP | 上 |
| 2 | ABE_RIGHT（默认） | 右 |
| 3 | ABE_BOTTOM | 下 |

### release_on_hide_mode / release_on_drag_mode

| 值 | hide_mode 含义 | drag_mode 含义 |
|----|-----------------|-----------------|
| 0 | 隐藏时保持贴边（不释放） | 拖动后保持贴边 |
| 1 | 释放 AppBar，保留 mode，恢复时重建（默认） | 释放 AppBar，转为浮动置顶（默认） |
| 2 | 释放 AppBar 并清除 mode，恢复时不贴边 | 释放 AppBar，转为普通窗口 |

### 注意事项

- 文件编码必须为 UTF-16 LE，否则程序拒绝读取并报错
- 文件不存在或为空时不报错，使用各字段默认值
- 程序在保存时重写整个文件，手动写入的注释行会被覆盖丢失
- 各字段读取时缺失则使用默认值，不会造成崩溃

相关源码：`src/storage/state_store.h`（StateData 结构体）、`src/storage/state_store.c`（Load/Save 实现）、`src/app/app.h`（枚举定义）

## 文档入口

- `README.md`：项目当前状态与构建方式
- `docs\design\architecture.md`：当前分层边界与架构说明
- `docs\readme.md`：`docs\` 目录维护规则
- `.ai\plan.md`：当前执行计划入口
- `.ai\standards\phase-planning.md`：阶段读取 / 制定标准

> `docs\plan\` 下的旧计划文档和 `docs\design\PL-*.md` 目前保留为历史材料，不是当前执行依据。

## License

- 项目主体许可待定
- `lib\md4c` 使用 MIT 协议
