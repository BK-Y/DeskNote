# Shell-5a_repair-5-v — 生命周期释放（hide / drag）与配置化行为

## ① 核心问题

- 在 edge_reserved（贴边）模式下：
  - 用户隐藏到托盘时，AppBar 仍然占用系统保留区（未释放），可能导致工作区长期受限；
  - 用户拖动窗口并释放后，有时需要释放 AppBar（并变为浮动或普通），但当前没有可配置的策略；
  - 用户期望控制这些行为（恢复或放弃贴边）应可配置并持久化。

## ② 目标

- 新增两个配置项存于 state.ini：
  - `release_on_hide_mode`：0=never，1=release-and-remember，2=release-and-clear（默认 1）
  - `release_on_drag_mode`：0=never，1=release->FLOATING_TOPMOST，2=release->NONE（默认 1）
- 根据配置在 `App_HideToTray` 和窗口拖动结束时执行对应行为（Unregister / state 更新 / mode 转换）。

## ③ 设计要点

- 遵守分层：storage 负责配置持久化，app 层负责决策，platform 层执行 AppBar 接口。
- 保持向后兼容：若 state.ini 中无这些字段，使用默认值（1）。
- release-and-remember（1）依赖 5b 的异常恢复（重注册）以保证在系统事件后能重建 AppBar。

## ④ 具体修改点（示意）

### 1) state_store

- 在 `StateData` 结构中新增：
```c
int release_on_hide_mode;   // 0/1/2
int release_on_drag_mode;   // 0/1/2
```
- `StateStore_Load`：若字段不存在（旧文件），返回默认值 1；`StateStore_Save` 持久化数值。

### 2) App_HideToTray (src/app/app.c)

在现有 `App_HideToTray` 中插入：

```c
StateData state;
StateStore_Load(&state);
if (state.release_on_hide_mode == 1) {
    /* release-and-remember */
    if (AppBar_IsRegistered(hwnd))
        AppBar_Unregister(hwnd);
    /* do not change g_app.shell.resident_mode nor state.shell_resident_mode */
} else if (state.release_on_hide_mode == 2) {
    /* release-and-clear */
    if (AppBar_IsRegistered(hwnd))
        AppBar_Unregister(hwnd);
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
    state.shell_resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
    StateStore_Save(&state);
}
ShowWindow(hwnd, SW_HIDE);
/* persist presence as before */
```

### 3) Drag end handler (src/platform/win32/window.c)

在 `WndProc` 新增对 `WM_EXITSIZEMOVE`（或 `WM_MOVING`/`WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE` 组合）的处理：

```c
case WM_EXITSIZEMOVE:
{
    if (App_GetResidentMode() == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
    {
        StateData st; StateStore_Load(&st);
        if (st.release_on_drag_mode == 1) {
            /* release -> floating_topmost */
            RECT rc; GetWindowRect(hwnd, &rc);
            st.last_floating_left = rc.left; st.last_floating_top = rc.top;
            st.last_floating_width = rc.right - rc.left; st.last_floating_height = rc.bottom - rc.top;
            StateStore_Save(&st);
            if (AppBar_IsRegistered(hwnd)) AppBar_Unregister(hwnd);
            SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
            App_SetResidentMode(APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST);
        } else if (st.release_on_drag_mode == 2) {
            if (AppBar_IsRegistered(hwnd)) AppBar_Unregister(hwnd);
            App_SetResidentMode(APP_SHELL_RESIDENT_MODE_NONE);
            st.shell_resident_mode = APP_SHELL_RESIDENT_MODE_NONE; StateStore_Save(&st);
        }
    }
}
break;
```

## ⑤ 修改文件清单

- `src/storage/state_store.h` / `state_store.c` : 新增两个字段读写
- `src/app/app.c` : 修改 `App_HideToTray` 行为
- `src/platform/win32/window.c` : 新增 `WM_EXITSIZEMOVE` 处理
- `src/app/app.h` : 如需，公开 `App_SetResidentMode`（已存在）

## ⑥ 推进步骤

1. 修改 `state_store.h/c`，新增字段并实现默认值逻辑；编译通过。
2. 修改 `app.c` 的 `App_HideToTray`，实现 release_on_hide_mode 的三路策略；编译通过。
3. 修改 `window.c`，在 WndProc 中 handle `WM_EXITSIZEMOVE`，实现 release_on_drag_mode 行为；编译通过。
4. 测试：
   - `release_on_hide_mode=1`：隐藏时 AppBar_Unregister 被调用，恢复后（在 5b/repair 或 startup 再现时）能重建；
   - `release_on_hide_mode=2`：隐藏时 AppBar_Unregister 并把 resident_mode 设为 NONE；恢复时不再重建；
   - `release_on_drag_mode=1`：拖动结束后窗口变为 floating_topmost，last_floating_* 保存；
   - `release_on_drag_mode=2`：拖动结束后 resident_mode 清空，AppBar_Unregister 被调用；
5. 记录日志与 edge 情况（AppBar_Register 返回失败时的容错路径）。

## ⑦ 验收标准

- 覆盖上文 4 种组合的行为（hide/drag × mode）。
- 分层核实通过。
- 在 release-and-remember 模式下，5b 的异常恢复能在系统事件驱动时完成重建（需后续 5b 验证）。

## ⑧ 风险与注意

- release-and-remember 强依赖 5b 的重注册能力；如果 5b 未就绪且系统在隐藏期间修改了工作区，恢复可能失败。
- WM_EXITSIZEMOVE 中并发 ABM 回调需要 AppBar 模块的幂等/并发安全性。
- 若用户把 release_on_* 设置为 aggressive（2），可能导致用户迷惑：隐藏后窗口再也不贴边，需在 UI/帮助中提示。