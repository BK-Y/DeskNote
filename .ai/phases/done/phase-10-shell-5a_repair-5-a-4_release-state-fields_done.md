# phase-10-shell-5a_repair-5-a-4 — 新增 state 字段：release_on_hide_mode / release_on_drag_mode

目标

- 在 StateStore 中添加两个新字段：`release_on_hide_mode` 和 `release_on_drag_mode`（默认值 1），为后续可配置的"隐藏时释放贴边"和"拖动时释放贴边"行为提供持久化支持。

范围（单一小改动）

- 修改 `src/storage/state_store.h` / `src/storage/state_store.c`：
  - 在 `StateData` 中新增 `int release_on_hide_mode` 和 `int release_on_drag_mode`。
  - 在 `StateStore_Load` 中为旧 ini 文件提供默认值（1）。
  - 在 `StateStore_Save` 中将新字段写入 ini。

实现步骤

1. 在 `state_store.h` 的 `StateData` 结构体末尾新增：
   ```c
   int release_on_hide_mode;   /* 0=never, 1=release-and-remember(default), 2=release-and-clear */
   int release_on_drag_mode;   /* 0=never, 1=release->FLOATING_TOPMOST(default), 2=release->NONE */
   ```
2. 在 `state_store.c` 的 `StateStore_Load` 中：若 ini 中缺少对应键，则字段默认设为 1。
3. 在 `StateStore_Save` 中：将两个字段写入 ini 的对应节。
4. 行为层面的使用（在 App_HideToTray / WM_EXITSIZEMOVE 中的切换逻辑）不在本计划范围内，将在后续独立计划中实现。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| E-1 | 全新 state.ini 保存后写入新字段 | 删除 state.ini（或用备份旧文件） | 启动 desknote，触发一次 StateStore_Save（如切换菜单） | `state.ini` 中出现 `release_on_hide_mode=1` 和 `release_on_drag_mode=1` |

### 边界条件
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| E-2 | 旧 state.ini 无新字段→加载默认值 | 使用旧版 state.ini（不含 release_* 行） | 启动 desknote，读一次 state | 代码中 `state.release_on_hide_mode == 1` 且 `state.release_on_drag_mode == 1` |
| E-3 | 手动设非法值→保存后恢复为显式值 | state.ini 中写 `release_on_hide_mode=5` | 启动 → 触发保存 | 写入后仍为 5（StateStore_Save 不做校验，原样持久化） |

### 错误处理
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| E-4 | state.ini 损坏/空文件 | state.ini 为空文件 | 启动 desknote | 默认值设为 1，加载不崩溃 |

### 回归
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| E-5 | 现有字段读写不受影响 | 正常 state.ini | 启动→保存→检查 | `shell_resident_mode`、`dock_edge` 等已有字段值不变 |
| E-6 | 编译通过 | 代码已修改 | `cmake --build build` | 零错误零警告 |

### 集成
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| E-7 | 全字段 Save→Load 一致性 | 所有 state 字段有值 | 1) `StateStore_Load` → 2) 修改几个字段 → 3) `StateStore_Save` → 4) 重新 Load | Load 后的值与 Save 前的值完全一致 |

依赖

- 无。

预计耗时

- 20–40 分钟。
