# phase-10-shell-5a_repair-5-a-1 — 启动时立即注册 AppBar

目标

- 在应用启动流程中（App_Init），如果持久化状态表明 `shell_resident_mode == EDGE_RESERVED`，立即通过平台的公共 API 注册 AppBar（`AppBar_Register` + `AppBar_SetPosition`）。

范围（单一小改动）

- 在 `src/app/app.c` 中新增一个小函数（例如 `App_TryRegisterAppBarFromState(HWND hwnd)`），并在 `App_Init` 创建/显示窗口后调用它。
- 不修改 platform 层实现（只调用平台公开函数）。

实现步骤

1. 在 `src/app/app.c` 添加 `static void App_TryRegisterAppBarFromState(HWND hwnd)`：
   - 读取当前持久化状态（通过现有 StateData / StateStore API），检查 `shell_resident_mode`。
   - 若为 `EDGE_RESERVED`，读取 `dock_edge`、`dock_thickness` 等配置并做范围校验/clamp（见计划 repair-5-a-5）。
   - 调用 `AppBar_Register(hwnd, edge, thickness)`（平台公开 API）。根据返回值记录日志（`OutputDebugStringW`），但不要更改持久化状态。
   - 在成功注册后调用 `AppBar_SetPosition(hwnd)` 或平台提供的调整位置 API。
2. 在 `App_Init` 的窗口创建完成处（保证有 HWND）调用 `App_TryRegisterAppBarFromState(hwnd)`。
3. 日志策略：使用统一前缀（如 `[s5a-r5a1]`），记录尝试/成功/失败及错误码。
4. 错误处理原则：失败时仅记录，不在 app 端清除或修改 `shell_resident_mode`，由 platform handler（5b）负责后续重试。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| B-1 | 启动恢复右贴边 240px | `state.ini` 中 `shell_resident_mode=2\ndock_edge=2\ndock_thickness=240` | 启动 desknote | 1) DebugView 出现 `[s5a-r5a] App_TryRegisterAppBarFromState: edge=2 thickness=240`<br>2) 出现 `[s5a-r5a] ... success`<br>3) 窗口贴右边缘，宽 240px |

### 边界条件
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| B-2 | 厚度低于下限被钳位 | `dock_thickness=50` | 启动 | 日志中 `thickness=200`（被 ClampDockThickness 钳位） |
| B-3 | 方向值非法被钳位 | `dock_edge=-1` | 启动 | 日志中 `edge=2`（被 ClampDockEdge 钳位到 APP_DOCK_RIGHT） |
| B-4 | NONE 模式不注册 | `shell_resident_mode=0` | 启动 | 无 `[s5a-r5a]` 注册日志 |

### 错误处理
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| B-5 | AppBar_Register 失败时保留 state | `shell_resident_mode=2`，`AppBar_Register` 返回失败 | 启动 | 1) 日志出现 `AppBar_Register failed, state preserved`<br>2) `state.ini` 中 `shell_resident_mode` 仍为 2<br>3) 下次启动仍尝试注册 |

### 回归
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| B-6 | 浮动置顶启动恢复不受影响 | `shell_resident_mode=1` + last_floating_* 有值 | 启动 | 窗口置顶在 last_floating_* 位置，无 `[s5a-r5a]` 注册日志 |
| B-7 | 正常启动无 state 不崩溃 | state.ini 不含 shell_resident_mode 字段 | 启动 | 应用正常启动，窗口正常显示 |

### 集成
| 编号 | 用例名 | 前置条件 | 操作步骤 | 预期结果 |
|------|--------|---------|---------|---------|
| B-8 | 启动贴边→隐藏→再次启动 | B-1 执行后，退出，不改 state.ini | 重新启动 | 再次出现 B-1 的全部预期日志和贴边行为 |

依赖与注意事项

- 该计划依赖尽快实现 platform 消息处理（见 `phase-10-shell-5b.md`），否则在 Explorer 重启/显示变更后可能无法恢复。建议将此计划与 5b 并行实施。

预计耗时

- 30–60 分钟（包含编译与一次手动验证）。
