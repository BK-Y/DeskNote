# Plan-11 — 统一配置入口

## 核心问题

当前配置读写分散。`app.c` 中散落多处 `StateStore_Load` / `StateStore_Save` 调用，写注释全丢。
`shell_resident_mode` 在被 8 处独立写入，AppBar 注册/注销路径不统一。

## 架构

```
调用方（app.c → shell 模块）
         │
         ├─ Config_Get("key", default)     ← 读
         ├─ Config_Set("key", val)          ← 唯一写入口
         └─ Config_OnChange(callback)       ← 件数行为注册
                  │
         ┌────────┴────────┐
         │   config 层      │  内存表 + 件数行为
         │   config.c/.h   │
         └────────┬────────┘
                  │
         ┌────────┴────────┐
         │   ini2arr 层     │  ini 文件读写
         │   utils/ini2arr │  行级增改，注释保留
         └────────┬────────┘
                  │
              state.ini
```

config 层除了 ini2arr 不依赖任何业务代码（app/platform/editor/ui/render）。
全部读写只调 Config_Get / Config_Set，不再调用 StateStore。

## 方案选型

### 候选方案

- **方案 A（选中）：新增 config 层**
  - 在 ini2arr 之上建立 config 层，作为单例运行时配置表
  - 调用方只调 Config_Get / Config_Set，不直接操作 ini 文件或 StateData
  - 完全符合当前架构分层规则：config 层仅依赖 utils，app 层依赖 config，不产生反向调用
  - 新增模块：`src/config/config.c` + `config.h`

- **方案 B（排除）：在 state_store 层直接改行级写入**
  - 不改 StateStore_Save 的调用方，只在底层改为行级写入保留注释
  - 排除理由：不解决 shell_resident_mode 8 处写入的核心问题。调用方依然直接写 StateData，件数行为依然分散

- **方案 C（排除）：继续使用当前架构，逐一修每处写作点**
  - 不新增模块，直接在各调用点修写作逻辑
  - 排除理由：和方案 B 一样不解决统一写入口的问题，后续新增配置项时依然需要人肉保证走同一个写路径

## 主链路

### 读路径（启动时）
```
App_Init → Config_Init → ini2arr("state.ini")
                         → 内存 entries 表
                         → Config_Get("shell_resident_mode")
                         → AppBar_Register / SetWindowPos
```

### 写路径（运行时）
```
菜单 cmd103 → Config_Set("shell_resident_mode", EDGE_RESERVED)
             ├─ 更新内存 entries
             ├─ ini2arr_write → .tmp → rename → state.ini
             └─ on_change 回调 → AppBar_Register + AppBar_SetPosition
```

## 子阶段

| 子阶段 | 状态 | 目标 | 产出 | 详细文件 |
|------|------|------|------|------|
| 11a | 已完成 | 模块骨架——建文件定接口，编译通过 | config.h + config.c 创建，语法验证通过 | `plan-11a-config-skeleton_done.md` |
| 11b | ini2arr 接入——Config_Init 读盘、Config_Set 写回 | 全部 4 API 实现，Config 单元测试通过 | `plan-11b-ini2arr-wiring.md` |
| 11c | 迁移写入点——app.c 中 9 处写入点逐一替换 | app.c 中 StateStore_Save 仅剩文档管理一行 | `plan-11c-migrate-writes.md` |
| 11c-01 | 启动恢复 | Config_Get 替代 StateStore 读启动路径 | `plan-11c-01-startup-restore.md` |
| 11c-02 | 菜单进入/退出贴边 | cmd103 改用 Config_Set | `plan-11c-02-menu-edge-reserved.md` |
| 11c-03 | 菜单浮动置顶 | cmd102 改用 Config_Set | `plan-11c-03-menu-floating.md` |
| 11c-04 | cmd103 保存浮动几何 | cmd103 浮动几何改用 Config_Set | `plan-11c-04-cmd103-save-geom.md` |
| 11c-05 | 拖拽结束 | App_OnEndDrag 改用 Config_Set | `plan-11c-05-drag-end.md` |
| 11c-06 | 托盘隐藏/恢复 | Hide/Restore 改用 Config_Set | `plan-11c-06-tray-hide-restore.md` |
| 11d | 回调注册与收尾——on_change 绑定、砍冗余函数 | 6 项回归全部通过 | `plan-11d-callbacks-cleanup.md` |

## 改写范围

| 文件 | 操作 |
|---|---|
| `src/config/config.c` | 新建 |
| `src/config/config.h` | 新建 |
| `CMakeLists.txt` | 加 config.c + ini2arr.c |
| `src/app/app.c` | 9 处替换 + 删除冗余函数 |
| `src/storage/state_store.*` | 不动（文档字段保留） |
| `src/utils/ini2arr.*` | 不动（已完成） |

## 键定义

```c
#define KEY_SHELL_RESIDENT_MODE  "shell_resident_mode"
#define KEY_DOCK_EDGE            "dock_edge"
#define KEY_DOCK_THICKNESS       "dock_thickness"
#define KEY_PRESENCE_STATE       "presence_state"
#define KEY_FLOATING_LEFT        "last_floating_left"
#define KEY_FLOATING_TOP         "last_floating_top"
#define KEY_FLOATING_WIDTH       "last_floating_width"
#define KEY_FLOATING_HEIGHT      "last_floating_height"
#define KEY_TITLEBAR_HEIGHT      "titlebar_height"
#define KEY_FRAME_VISUAL_THICK   "frame_visual_thickness"

/* 默认值 */
#define CONFIG_DEFAULT_TITLEBAR_HEIGHT        32
#define CONFIG_DEFAULT_FRAME_VISUAL_THICKNESS   1
#define CONFIG_DEFAULT_FRAME_RESIZE_THICKNESS   6
#define CONFIG_DEFAULT_DOCK_EDGE                2  /* APP_DOCK_RIGHT */
#define CONFIG_DEFAULT_DOCK_THICKNESS         240
#define CONFIG_DEFAULT_WINDOW_WIDTH           240
#define CONFIG_DEFAULT_WINDOW_HEIGHT          388
```

## 现状分析

| 配置项 | 写位置 | 读位置 | 件数行为 |
|------|------|------|------|
| shell_resident_mode | 8 处 | 10+ 处 | AppBar/Z 序 |
| dock_edge | cmd103 | App_TryRegister ✓ | AppBar_SetPosition |
| dock_thickness | cmd103（硬编码 240） | App_TryRegister / AppBar | AppBar_SetPosition |
| last_floating_*（4项） | cmd102/cmd103/OnEndDrag 三处 | App_Init | SetWindowPos |
| presence_state | Hide/RestoreTray | App_Init | ShowWindow |

## 完成标准

- [ ] Plan-11a: `src/config/config.h` + `src/config/config.c` 创建，`app.c` 可 `#include` 编译通过
- [ ] Plan-11b: `Config_Init` 从 state.ini 加载 → Config_Get 读出正确值 → Config_Set 写入后文件注释保留。测试全部通过
- [ ] Plan-11c: `app.c` 中 9 处写入点全部替换为 Config_Set，`grep "StateStore_Save" src/app/app.c` 仅剩文档管理一行
- [ ] Plan-11d: 注册 on_change 回调，Config_Set("shell_resident_mode") 自动触发 AppBar 注册/注销。全部 6 项回归验证通过
- [ ] 分层核实：config 层不反向依赖 app/platform/editor/ui/render/storage

## 关联

- `src/utils/ini2arr.c/.h`（已完成，测试通过）
- `.ai/insights/it-001-ini2arr.md`
