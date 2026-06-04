# Plan-11d — 回调注册与收尾

## 目标
注册 on_change 回调，让 Config_Set("shell_resident_mode", ...) 自动触发 AppBar ops 和 Z 序调整。
删除 app.c 中已被 Config 替代的冗余函数。

## 前提
11c 全部 9 步替换完成。app.c 中所有写入点已改用 Config_Set。

## 步骤 1：注册 on_change 回调

在 `app.c` 中新增：

```c
static void OnShellModeChanged(const char* key, int old_val, int new_val)
{
    (void)key;
    if (old_val == new_val) return;
    HWND hwnd = g_app.hwnd;

    /* exit old mode */
    if (old_val == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
        AppBar_Unregister(hwnd);
    if (old_val == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

    /* enter new mode */
    if (new_val == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED) {
        int edge = Config_Get(KEY_DOCK_EDGE, APP_DOCK_RIGHT);
        int thick = Config_Get(KEY_DOCK_THICKNESS, 240);
        AppBar_Register(hwnd);
        AppBar_SetPosition(hwnd, (AppDockEdge)edge, thick);
    }
    if (new_val == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
}
```

在 `App_Init` 中注册：

```c
Config_OnChange(OnShellModeChanged);
```

## 步骤 2：移除各迁移步骤中残留的 AppBar ops 调用

回顾 11c 中 9 步，部分步骤还保留了显式 AppBar ops 调用（用于保证每步独立验证）。11d 完成后，这些可以从调用点删除，因为 Config_Set 已自动触发。

| 11c 子计划 | 可删除的代码 |
|---------|------------|
| 11c-01（启动恢复） | `AppBar_Register + AppBar_SetPosition` — on_change 未被触发（启动只读），保留 |
| 11c-02 Step A（cmd103 进入贴边） | `AppBar_Register + AppBar_SetPosition` → 由 on_change 接管 |
| 11c-02 Step B（cmd103 退出贴边） | `AppBar_Unregister` → 由 on_change 接管 |
| 11c-05 Step A（拖拽→浮动） | `AppBar_Unregister` → 由 on_change 接管 |
| 11c-05 Step B（拖拽→普通） | `AppBar_Unregister` → 由 on_change 接管 |

## 步骤 3：删除冗余函数

| 删除 | 原因 |
|---|---|
| `App_TryRegisterAppBarFromState` | 启动恢复已改用 Config_Get，不再有单独的"从 state 恢复"函数 |
| `App_WriteShellStateData` | 不再需要把 g_app.shell 字段写回 StateData（Config 层直接管行级写） |
| `AppBar_ReadDockConfig` 中的 StateStore_Load | 改为 Config_Get |

## 步骤 4：删除 g_app.shell.resident_mode 的直接写

能删除的直接写入点（11c 已保留兼容，本步骤清掉）：

| 位置 | 当前 | 改为 |
|------|------|------|
| cmd103 | `g_app.shell.resident_mode = EDGE_RESERVED` | Config_Set 已触发，不需要重复写 |
| cmd103 退出 | `g_app.shell.resident_mode = NONE` | 同 |
| App_OnEndDrag | `g_app.shell.resident_mode = FLOATING` | 同 |

保留 `g_app.shell.resident_mode` 作为运行时副本（多处直接读它做菜单复选框判断等），但写入口统一到 on_change 回调中。

## 验证

全部功能逐一回归：

| 功能 | 操作 | 预期 |
|------|------|------|
| 冷启动贴边 | shell_resident_mode=2 重启 | AppBar 注册，贴边 |
| 冷启动浮动 | shell_resident_mode=1 + floating 坐标 | 窗口浮动到记录位置 |
| 菜单→贴边 | 点"贴边占位" | Config_Set + on_change → AppBar ops |
| 菜单→浮动 | 点"浮动置顶" | Config_Set + on_change → HWND_TOPMOST |
| 拖拽→浮动 | 贴边拖开 | Config_Set + on_change → 释放 AppBar + 浮动 |
| 托盘隐藏/恢复 | 贴边→隐藏→恢复 | 恢复后状态正确 |

## 分层核实

- AppBar ops 集中到 on_change 回调中（位于 app 层，直到 shell 拆分完成后移入 shell 模块）
- 本子计划不修改：src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
- 删除的 `App_TryRegisterAppBarFromState` 和 `App_WriteShellStateData` 属于 app 层内部重构

## 风险

- 删除 `g_app.shell.resident_mode` 的直接写后，如果某个路径遗漏了 Config_Set 调用，模式不会更新。验证表 6 项全部 pass 即可确认。
- 删除的文件 `App_TryRegisterAppBarFromState` 后续可能被 Plan-10 子阶段引用，删除时需确认所有引用者已替换。

最终状态：app.c 中对 g_app.shell.resident_mode 的写入全部通过 Config_Set 路径，AppBar ops 全部通过 on_change 回调触发，StateStore_Save 仅剩 App_SaveCurrentDocument 中的文档字段。
