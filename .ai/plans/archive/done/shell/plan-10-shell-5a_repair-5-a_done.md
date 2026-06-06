# Shell-5a_repair-5-a — 启动与托盘恢复（App_Init / App_RestoreFromTray）

## ① 核心问题

- 贴边（edge_reserved）模式在启动时和从托盘恢复时无法自动重建 AppBar：
  - 启动时如果上次状态是贴边，程序没有注册 AppBar 并贴边；
  - 从托盘恢复时如果之前处于贴边，程序没有重建 AppBar。

## ② 目标

- 启动时：若 `state.shell_resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED`，自动调用 `AppBar_Register` + `AppBar_SetPosition` 并把运行态 `g_app.shell.resident_mode` 设为 `EDGE_RESERVED`（在注册成功时）。
- 托盘恢复时：若 `g_app.shell.resident_mode == EDGE_RESERVED`，读 `state.dock_edge`/`state.dock_thickness` 并尝试 `AppBar_Register` + `AppBar_SetPosition`。

> 备注：若注册失败（系统拒绝或资源问题），不要在此阶段清空 `state.shell_resident_mode`；应由异常恢复（5b）负责后续重试/补救。

## ③ 方案设计（含备选与选中项）

本问题的关键是“何时调用 `AppBar_Register`”与如何保证系统回调不丢失。列出两种可行方案：

- 方案 A（已选，Recommended）—— 立即注册
  - 行为：在 `App_Init`（初始化流程内）直接调用 `AppBar_Register` + `AppBar_SetPosition`（如果 state 表示之前为贴边）。
  - 优点：实现最简单、入侵最小；符合“修补”性质；app 层只调用 platform 公开接口，分层合规。
  - 缺点：若 platform（WndProc）的回调 handler（`WM_APP+2` / `TaskbarCreated`）尚未接入，系统短期回调可能被丢失；需要并行保证 5b 的 handler 尽快就绪或在失败时由 5b 重试。

- 方案 B（备选）—— 延迟注册（在窗口事件循环稳定后再注册）
  - 行为：在 `ShowWindow/UpdateWindow` 之后或首次 WM_TIMER tick 中再调用 `AppBar_Register`。
  - 优点：避免短时回调丢失（WndProc 已就绪）。
  - 缺点：实现更复杂（需额外调度逻辑），改动更多，增加测试成本。

**本修复采用方案 A（立即注册）作为默认策略**，同时在计划里硬性要求：
- 在同一开发周期/PR 中尽快完成 5b 中的 platform handler（`WM_APP+2` / `TaskbarCreated` / `WM_DISPLAYCHANGE` / `WM_SETTINGCHANGE`），或把 5b 的 handler 作为并行变更提交；
- 在 `AppBar_Register` 失败时记录诊断日志并保留持久化状态，由 5b 做重试。

## ④ 实现细节（严格分层、容错与诊断）

实施时必须遵守下面的约束：

- 严格分层：app 层仅调用 `AppBar_Register` / `AppBar_SetPosition` 等 platform 公开接口；platform 层负责所有与 SHAppBarMessage/系统消息交互。
- 参数校验与 clamp：从 `StateStore_Load` 读取 `state.dock_edge` / `state.dock_thickness` 后进行边界检查：
  - `edge` 必须在 [0,3]（`APP_DOCK_LEFT/TOP/RIGHT/BOTTOM`），否则使用 `APP_DOCK_RIGHT` 作为默认。
  - `thickness` 在 [200, 500] 范围内，否则 clamp 到该范围，默认 240。
- 日志与诊断（必须）：
  - 在调用前后打印可追踪的调试字符串（使用 `OutputDebugStringW`），示例：
    - "[s5a-repair5a] restoring EDGE_RESERVED edge=%d thickness=%d\n"
    - "[s5a-repair5a] AppBar_Register failed: err=%d\n"（若可获取错误码）
  - 保留原有 `[s5a-diag]` 风格，便于 DebugView 统一抓取。
- 注册失败处理：
  - 若 `AppBar_Register()` 返回非 0：记录日志，但**不要**清除 `state.shell_resident_mode` 或 `g_app.shell.resident_mode`；让后续 5b 的重注册机制（TaskbarCreated/WM_DISPLAYCHANGE）尝试重建。
- 成功后的状态同步：仅在 `AppBar_Register` 成功且 `AppBar_SetPosition` 执行成功后才把 `g_app.shell.resident_mode` 置为 `EDGE_RESERVED`（避免不一致）。

### 建议示意（含校验与日志）

```c
StateData state;
StateStore_Load(&state);
AppDockEdge edge = (AppDockEdge)state.dock_edge;
int thickness = state.dock_thickness;
// validate
if (edge < APP_DOCK_LEFT || edge > APP_DOCK_BOTTOM) edge = APP_DOCK_RIGHT;
if (thickness < 200) thickness = 200;
if (thickness > 500) thickness = 500;

wchar_t buf[128];
swprintf(buf, 128, L"[s5a-repair5a] attempt restore edge=%d thickness=%d\n", edge, thickness);
OutputDebugStringW(buf);

if (AppBar_Register(g_app.hwnd) == 0)
{
    AppBar_SetPosition(g_app.hwnd, edge, thickness);
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED; // only after success
}
else
{
    swprintf(buf, 128, L"[s5a-repair5a] AppBar_Register failed\n");
    OutputDebugStringW(buf);
    // leave state intact; 5b will retry
}
```

## ⑤ 推进步骤（细化）

1. 在本地分支实现代码修改（`src/app/app.c` 两处）：
   - `App_Init` 的 `Shell-4a_2` 后插入启动恢复逻辑（含参数校验 + 日志 + 失败处理）。
   - `App_RestoreFromTray` 的浮动恢复后插入托盘恢复逻辑（同上）。
2. 编译（`cmake --build build`），处理编译错误。
3. 本地功能验证（开发机）：
   - 将 `state.ini` 中 `shell_resident_mode` 写为 `EDGE_RESERVED`（或通过 UI 进入贴边并重启），启动后观察：是否有 `[s5a-repair5a] attempt restore` 日志与 `setpos` 类日志，窗口是否贴边。
   - 贴边后隐藏到托盘再恢复，观察是否有注册/设置日志与贴边行为。
   - 若 `AppBar_Register` 失败，观察日志并确保 state 未被清空。
4. 在代码 PR 中注明：此修复依赖 5b（平台 handler）在短期内完成以保障稳健性。
5. 若验证通过，合并；否则回滚本次改动并在 `phase-10-shell-5a_repair-5-a.md` 中记录失败观察与下一步诊断。

## ⑥ 验收标准（更细化）

- 启动恢复：当 `state.shell_resident_mode == EDGE_RESERVED` 时，应用应输出类似：
  - `[s5a-repair5a] attempt restore edge=%d thickness=%d`，随后至少有 `setpos_rc` 或相关 AppBar 日志且窗口贴边。 
- 托盘恢复：Hide→Restore 路径，应输出相同类型日志并恢复贴边。
- 错误处理：若 `AppBar_Register` 失败，应输出 `[s5a-repair5a] AppBar_Register failed`，且不清理 state（便于 5b 重试）。
- 分层核实：app 层只使用 `AppBar_*` 接口，不触及 platform 内部状态；所有新增日志为诊断辅助信息。

## ⑦ 风险与回退

- **回调丢失风险**（若 platform 的 handler 尚未就绪）：如果短期内未实现 5b 的 handler，注册后的某些系统回调可能在短窗口内被丢失。缓解：实现后续的 5b handler 并在失败时通过日志发现与重试。
- **注册失败**：AppBar_Register 可能在某些环境失败（权限/系统差异）。策略：记录失败，不清理持久态，交由 5b 重试或人工诊断（DebugView）处理。
- **回退**：回退为单个 commit 回滚 `app.c` 的插入代码；repair 文档保留用于审计。

---

如同意该增强版本，我会把 `phase-10-shell-5a_repair-5-a.md` 更新（我已经准备好做修改），并随后把 `5a_repair-5-b` / `5a_repair-5-v` 同样补强日志/校验/容错项。需要我现在把该增强文本写入文件（覆盖现有）并列入变更列表吗？