#!/usr/bin/env python3
# Append state.ini reference section to README.md
content = open('README.md', 'r', encoding='utf-8').read()
section = """

## state.ini 配置项参考

文件路径: `%LOCALAPPDATA%\\DeskNote\\state.ini`
编码: UTF-16 LE (Windows "Unicode")

### 字段总表

| 键名 | 类型 | 值范围 | 引入阶段 | 说明 |
|------|------|-------|---------|------|
| `version` | int | 固定 2 | 初始 | 格式版本号 |
| `last_file` | 路径 | — | 初始 | 上次打开的文档路径 |
| `use_custom_chrome` | 0/1 | 固定 1 | 初始 | 自定义标题栏(不可改) |
| `titlebar_height` | int | 默认 32 | 初始 | 标题栏高度(px) |
| `frame_visual_thickness` | int | 默认 1 | 初始 | 边框视觉厚度(px) |
| `shell_resident_mode` | 0-2 | 0=NONE, 1=FLOATING_TOPMOST, 2=EDGE_RESERVED | Shell-3c_2 | 窗口常驻模式 |
| `presence_state` | 0-2 | 0=VISIBLE, 1=HIDDEN, 2=EXITING | Shell-3c_2 | 窗口可见状态 |
| `last_floating_*` | int | — | Shell-4a_2 | 上次浮动置顶时的窗口坐标与尺寸 |
| `dock_edge` | 0-3 | 0=左, 1=上, 2=右, 3=下 | Shell-5a | AppBar 贴边方向 |
| `dock_thickness` | int | 200-500(运行时clamp) | Shell-5a | 贴边保留区厚度(px) |
| `release_on_hide_mode` | 0-2 | 0=never, 1=remember, 2=clear | repair-5-a-4 | 隐藏时释放策略(默认1) |
| `release_on_drag_mode` | 0-2 | 0=never, 1=to_topmost, 2=to_none | repair-5-a-4 | 拖动时释放策略(默认1) |

### shell_resident_mode 详解

| 值 | 含义 | 行为 |
|----|------|------|
| 0 | NONE | 普通窗口 |
| 1 | FLOATING_TOPMOST | 浮动置顶 |
| 2 | EDGE_RESERVED | 注册为 AppBar 贴边, 启动/托盘恢复时自动注册 |

### dock_edge 枚举

| 值 | 对应 ABE_ | 贴边位置 |
|----|-----------|---------|
| 0 | ABE_LEFT | 左 |
| 1 | ABE_TOP | 上 |
| 2 | ABE_RIGHT(默认) | 右 |
| 3 | ABE_BOTTOM | 下 |

### release_on_hide_mode / release_on_drag_mode

| 值 | hide_mode 含义 | drag_mode 含义 |
|----|-----------------|-----------------|
| 0 | 隐藏时保持贴边(不释放) | 拖动后保持贴边 |
| 1 | 释放 AppBar, 保留 mode, 恢复时重建(默认) | 释放 AppBar, 转为浮动置顶(默认) |
| 2 | 释放 AppBar 并清除 mode, 恢复时不贴边 | 释放 AppBar, 转为普通窗口 |

### 注意事项

- 文件编码必须为 UTF-16 LE, 否则程序拒绝读取并报错
- 文件不存在或为空时不报错, 使用各字段默认值
- 程序在保存时重写整个文件, 手动写入的注释行会被覆盖丢失
- 各字段读取时缺失则使用默认值, 不会造成崩溃

相关源码: `src/storage/state_store.h` (StateData 结构体), `src/storage/state_store.c` (Load/Save 实现), `src/app/app.h` (枚举定义)
"""

open('README.md', 'w', encoding='utf-8').write(content + section)
print('done')
