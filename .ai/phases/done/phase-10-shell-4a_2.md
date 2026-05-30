# Shell-4a_2 — floating topmost 持久化（后续）

## ① 核心问题

浮动置顶模式可以手动切换了，但重启后模式丢失——窗口恢复为普通模式。上次浮动的位置也没有保存。

## ② 分析现状

### 阶段摘要

从"重启后 topmost 模式丢失"推进到"state.ini 保存 resident_mode 和 last_floating_bounds，启动后自动恢复"。

### 承接关系

前置阶段：Shell-4a_1（floating topmost 状态机已就绪，可切换置顶/取消置顶）

## ③ 方案设计

候选方案：
1. **state.ini 扩展字段** — 与 3c_2 一致的 Load→改字段→Save 模式
2. **新增独立的 bounds.ini** — 不必要的文件膨胀

选择方案 1 [selected]。理由：`shell_resident_mode` 已存在，只需新增 `last_floating_left`/`top`/`width`/`height` 字段，延续 state.ini 的现有模式。

## ④ 拆解执行

### 整体目标

1. state.ini 新增 last_floating_bounds 四个字段
2. 切换到 floating_topmost 时保存当前窗口位置
3. 切换回普通模式时保存当前窗口位置（供恢复时使用）
4. 启动时读取 resident_mode 和 last_floating_bounds，恢复窗口位置和层级

### 阶段产出

1. state.ini 中出现 last_floating_left/top/width/height 字段
2. 重启后窗口位置和置顶状态与上次退出前一致

## ⑤ 设定边界

### 本阶段范围

只做 topmost 的持久化。

### 不包括

1. 不做状态机逻辑（Shell-4a_1 已完成）
2. 不做 titlebar 菜单改动
3. 不改窗口显示/隐藏行为

### 分层归属

- **storage/** — state_store.h/c 中新增 last_floating_bounds 字段的读写
- **app/** — 在模式切换时触发生成 bounds 并保存；启动时读取并恢复
- **platform/** — 不参与

### 文件落点

#### 预计修改文件

- `src/storage/state_store.h` — 新增四个字段
- `src/storage/state_store.c` — 读写新字段
- `src/app/app.c` — 在模式切换时保存 bounds，启动时恢复

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/platform/win32/window.c`

## ⑥ 落地方案

### 技术路线

**state_store.h 新增字段：**

```c
/* 已有字段 ... */
int presence_state;

/* Shell-4a_2: 浮动窗口 bounds */
int last_floating_left;
int last_floating_top;
int last_floating_width;
int last_floating_height;
```

**state_store.c 读写：**

在 `StateStore_Load` 中新增解析：
```c
else if (wcsncmp(line_start, L"last_floating_left=", 19) == 0)
    out_state->last_floating_left = _wtoi(line_start + 19);
else if (wcsncmp(line_start, L"last_floating_top=", 18) == 0)
    out_state->last_floating_top = _wtoi(line_start + 18);
else if (wcsncmp(line_start, L"last_floating_width=", 20) == 0)
    out_state->last_floating_width = _wtoi(line_start + 20);
else if (wcsncmp(line_start, L"last_floating_height=", 21) == 0)
    out_state->last_floating_height = _wtoi(line_start + 21);
```

在 `StateStore_Save` 的 `swprintf` 中追加字段。

**app.c 在模式切换时保存 bounds：**

在 `App_SubmitShellCommand` 的 `SHOW_MENU` 命令处理中，`cmd == 102`（浮动置顶菜单项）分支：

```c
else if (cmd == 102) /* 浮动置顶 */
{
    /* 先保存当前窗口位置到 state.ini */
    RECT rect;
    if (GetWindowRect(g_app.hwnd, &rect))
    {
        StateData state;
        StateStore_Load(&state);
        state.last_floating_left   = rect.left;
        state.last_floating_top    = rect.top;
        state.last_floating_width  = rect.right - rect.left;
        state.last_floating_height = rect.bottom - rect.top;
        StateStore_Save(&state);
    }
    /* 提交切换命令，由 window.c 收束执行 SetWindowPos */
    App_SubmitShellCommand(APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST);
}
```

**app.c 启动时恢复：**

在 `App_Init` 中，presence 恢复逻辑之后、`App_EnsureCaretVisible` 之前插入：

```c
    /* Shell-4a_2: 启动时恢复浮动置顶状态 */
    {
        StateData state;
        StateStore_Load(&state);
        if (state.shell_resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        {
            SetWindowPos(hwnd, HWND_TOPMOST,
                        state.last_floating_left,
                        state.last_floating_top,
                        state.last_floating_width,
                        state.last_floating_height,
                        SWP_NOZORDER);
        }
    }
```

### 主链路

```text
用户切换浮动置顶
-> app 获取当前窗口位置
-> 保存到 state.ini（last_floating_*）
-> SetWindowPos(HWND_TOPMOST)
-> mode = floating_topmost

程序启动
-> App_Init
-> StateStore_Load
-> 检查 resident_mode
-> 如果是 floating_topmost → SetWindowPos 恢复窗口位置 + HWND_TOPMOST
```

## ⑦ 验收标准

1. state.ini 中出现 last_floating_left/top/width/height 字段
2. 置顶后关闭程序，重启后窗口回到上次位置并置顶
3. 非置顶状态关闭程序，重启后不影响（不错误地置顶）
4. 分层归属与文件落点一致

## ⑧ 推进步骤

### 方案 1：state.ini 扩展字段 [selected]

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/storage/state_store.h` 中 `StateData` 新增 `last_floating_left/top/width/height` 四个 int 字段 | 结构体字段就绪 | 编译通过 |
| 2 | `src/storage/state_store.c` 的 `StateStore_Load` 中新增四个字段的 `wcsncmp` 解析分支 | 启动时能从 ini 文件读 bounds | 编译通过 |
| 3 | `src/storage/state_store.c` 的 `StateStore_Save` 的 `swprintf` 中追加四个字段 | 切换时能写回 ini 文件 | 编译通过 |
| 4 | `src/app/app.c` 的 `cmd == 102`（浮动置顶菜单项）分支中，在 `App_SubmitShellCommand` 前插入 `GetWindowRect` → Load→写 bounds→Save——位置保存是 app 层的职责，window.c 只负责 SetWindowPos | 切换时自动保存 bounds | 编译通过 |
| 5 | `src/app/app.c` 的 `App_Init` 中，在 presence 恢复逻辑后检查 `shell_resident_mode`，如果是 `floating_topmost` 则用 `SetWindowPos` 恢复位置并置顶——启动时还原窗口状态 | 启动后自动恢复置顶状态和位置 | 编译通过，手动验证 |
| 6 | 验证：置顶→关闭→重启，窗口回到上次位置并置顶 | 恢复正确 | 手动验证 |
| 7 | 验证：非置顶→关闭→重启，窗口不错误地置顶 | 不产生脏状态 | 手动验证 |
