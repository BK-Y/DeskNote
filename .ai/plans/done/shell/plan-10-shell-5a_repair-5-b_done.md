# Shell-5a_repair-5-b — 菜单切换路径（floating_topmost → edge_reserved）与 last_floating_* 保存

## ① 核心问题

- 从浮动置顶（floating_topmost）切换到贴边占位（edge_reserved）时：
  - 未保存窗口的浮动位置（last_floating_*），导致以后恢复为浮动时缺少恢复点；
  - 未在切换前取消 topmost，可能导致窗口处于同时置顶且被注册为 AppBar 的冲突状态。

## ② 目标

- 在从 `FLOATING_TOPMOST` 切到 `EDGE_RESERVED` 前保存当前窗口 bounds 到 `state.last_floating_*`；
- 在切换动作前取消 topmost（`SetWindowPos(HWND_NOTOPMOST, ...)`）；
- 持久化 `dock_edge` / `dock_thickness` 并提交 `ENTER_EDGE_RESERVED` 命令。

## ③ 设计要点

- 小范围改动：仅在现有菜单处理分支（`App_SubmitShellCommand` 的 `SHOW_MENU` 处理）中补全；不增加新模块或新接口。
- 先保存 bounds，再取消 topmost，再设置 `g_app.shell.resident_mode` 并持久化，再提交 `ENTER_EDGE_RESERVED`。

## ④ 具体修改点（示意）

文件：`src/app/app.c`

在 `SHOW_MENU` 的 `cmd == 103` 分支（将要进入 `EDGE_RESERVED`）中：

```c
/* Shell-5a_repair-5-b: 如果当前是 FLOATING_TOPMOST，则先保存 last_floating_*，再取消 topmost */
if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
{
    RECT rect;
    if (GetWindowRect(g_app.hwnd, &rect))
    {
        StateData st;
        StateStore_Load(&st);
        st.last_floating_left = rect.left;
        st.last_floating_top = rect.top;
        st.last_floating_width = rect.right - rect.left;
        st.last_floating_height = rect.bottom - rect.top;
        StateStore_Save(&st);
    }

    /* 取消 topmost */
    SetWindowPos(g_app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/* 继续现有流程：切换 resident_mode -> 保存 dock config -> App_SubmitShellCommand(ENTER_EDGE_RESERVED) */
```

## ⑤ 修改文件清单

- 修改： `src/app/app.c`（`SHOW_MENU` 中 cmd==103 分支）
- 依赖： `src/storage/state_store.c/h`（使用现有 `StateStore_Load`/`Save`）

## ⑥ 推进步骤

1. 修改 `app.c`，插入保存+取消 topmost 逻辑。
2. 编译并修正编译问题。
3. 测试场景：先把窗口置为 floating topmost（通过菜单），然后选择“贴边占位”，验证：
   - last_floating_* 在 `state.ini` 中被正确保存；
   - 窗口先取消 topmost 再被 AppBar 注册并移到贴边位置；
   - 恢复浮动时能使用 last_floating_* 恢复位置。

## ⑦ 验收标准

- 切换路径操作流畅：先保存、先取消 topmost、再注册 AppBar 并贴边。
- `state.ini` 中 `last_floating_*` 确实更新。
- 编译通过。

## ⑧ 风险与注意

- 如果 `StateStore_Save` 失败，必须记录错误日志但仍继续切换，避免阻塞用户操作；
- SetWindowPos 取消 topmost 可能导致短暂闪烁；这是可以接受的交互代价。
