# Shell-3c_2 — 状态持久化（后续）

## ① 核心问题

重启应用后窗口状态全部丢失。上次是 visible_front 还是 hidden_to_tray、窗口在什么位置，一概不记得。

## ② 分析现状

### 阶段摘要

从"重启后全丢"推进到"state.ini 中保存 presence_state 和 resident_mode，启动时自动恢复"。

### 承接关系

前置阶段：Shell-3c_1（关闭策略已就绪，退出路径统一）

## ③ 方案设计

候选方案：
1. **state.ini 新增 presence_state / resident_mode 字段** — state.ini 已是现成的状态持久化文件，扩展字段即可
2. **新增独立的 state.json 文件** — 不必要的文件膨胀

选择方案 1。理由：state.ini 已在存储层封装好了读写接口，新增字段成本最低。

## ④ 拆解执行

### 整体目标

1. state.ini 新增字段：presence_state、resident_mode
2. 窗口进入 hidden_to_tray 时保存当前 presence 和 resident mode
3. 窗口退出时保存当前状态
4. 程序启动时从 state.ini 读取并恢复窗口状态
5. 异常退出时不保留可能冲突的 stale 状态

### 阶段产出

1. state.ini 中出现 `presence_state=visible_front` 或 `presence_state=hidden_to_tray`
2. 隐藏到托盘后，重启应用 → 窗口自动恢复到隐藏前的状态
3. 异常退出后不会读到冲突的状态数据

## ⑤ 设定边界

### 本阶段范围

只做 state.ini 的持久化字段。

### 不包括

1. 不做状态机逻辑（Shell-3b 已完成）
2. 不做关闭策略（Shell-3c_1 已完成）
3. 不改窗口显示/隐藏行为

### 分层归属

- **storage/** — state_store.h/c 中新增 presence_state、resident_mode 的读写
- **app/** — 在状态变化时调用 storage 接口触发持久化，启动时读取并恢复
- **platform/** — 不参与

### 文件落点

#### 预计修改文件

- `src/storage/state_store.h` — 新增 presence_state、resident_mode 字段
- `src/storage/state_store.c` — 读写新字段
- `src/app/app.c` — 在状态变化时触发持久化，启动时恢复

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/platform/win32/window.c`

## ⑥ 落地方案

### 技术路线

**state_store.h 新增字段：**

```c
/* 已有字段 ... */
int shell_resident_mode;

/* Shell-3c_2: 壳层状态持久化 */
int presence_state;      /* 0=visible_front, 1=hidden_to_tray, 2=exiting */
```

**state_store.c 读写：**

在 `StateStore_Load` 的解析链中新增：
```c
else if (wcsncmp(line_start, L"presence_state=", 15) == 0)
{
    out_state->presence_state = _wtoi(line_start + 15);
}
```

在 `StateStore_Save` 的 `swprintf` 格式化串中新增：
```c
L"shell_resident_mode=%d\r\n"
L"presence_state=%d\r\n",
/* ... */
state->shell_resident_mode,
state->presence_state);
```

**app.c 状态变化时触发持久化（与 App_EnableCustomChrome 一致的 Load→改字段→Save 模式）：**

在 `App_HideToTray` 中：
```c
void App_HideToTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_HIDDEN_TO_TRAY;
    ShowWindow(hwnd, SW_HIDE);

    /* 持久化当前 presence 状态 */
    StateData state;
    StateStore_Load(&state);
    state.presence_state = (int)SHELL_PRESENCE_HIDDEN_TO_TRAY;
    StateStore_Save(&state);
}
```

在 `App_RestoreFromTray` 中：
```c
void App_RestoreFromTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_VISIBLE_FRONT;
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    /* 持久化当前 presence 状态 */
    StateData state;
    StateStore_Load(&state);
    state.presence_state = (int)SHELL_PRESENCE_VISIBLE_FRONT;
    StateStore_Save(&state);
}
```

**app.c 启动时恢复：**

在 `App_Init` 中，`App_ApplyShellChromeState()` 之后、`App_EnsureCaretVisible()` 之前插入：

```c
    /* Shell-3c_2: 启动时恢复上次的 presence 状态 */
    {
        StateData state;
        StateStore_Load(&state);
        if (state.presence_state == 1) /* hidden_to_tray */
        {
            ShowWindow(hwnd, SW_HIDE);
        }
    }
```

### 主链路

```text
用户隐藏到托盘
-> App_HideToTray(hwnd)
-> 保存 presence_state = hidden_to_tray 到 state.ini
-> ShowWindow(SW_HIDE)
-> 窗口隐藏

用户从托盘恢复
-> App_RestoreFromTray(hwnd)
-> 保存 presence_state = visible_front 到 state.ini
-> ShowWindow(SW_SHOW) + SetForegroundWindow
-> 窗口恢复前台

程序启动
-> App_Init
-> StateStore_Load 加载 state.ini
-> 检查 presence_state
-> 如果是 hidden_to_tray → ShowWindow(SW_HIDE)
-> 如果是 visible_front → 正常显示
```

### 接口说明

无新增对外接口。内部通过 `StateData state; StateStore_Load(&state); ...; StateStore_Save(&state);` 局部变量模式实现，与 `App_EnableCustomChrome` 一致。app 层在状态变化时触发持久化。

## ⑦ 验收标准

1. state.ini 保存 presence_state 和 resident_mode
2. 手动关闭后重启，窗口恢复上次的 presence 状态
3. 异常退出后不会读到冲突的状态数据
4. 分层归属与文件落点一致

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/storage/state_store.h` 中 `StateData` 结构体新增 `int presence_state` 字段——与已有 `shell_resident_mode` 并列，不做全局单例，保持现有 Load/Save 模式 | 结构体字段就绪 | 编译通过 |
| 2 | `src/storage/state_store.c` 的 `StateStore_Load` 中新增 `presence_state` 的 `wcsncmp` 解析分支——与已有 `shell_resident_mode` 的解析方式一致 | 启动时能从 ini 文件读状态 | 编译通过 |
| 3 | `src/storage/state_store.c` 的 `StateStore_Save` 的 `swprintf` 格式化串中追加 `presence_state=%d\r\n`——与已有字段的写入方式一致 | 状态变化时能写回 ini 文件 | 编译通过 |
| 4 | `src/app/app.c` 的 `App_HideToTray` 中，ShowWindow 后追加 `StateData state; StateStore_Load(&state); state.presence_state = ...; StateStore_Save(&state);`——与 `App_EnableCustomChrome` 的持久化模式一致 | 隐藏时持久化状态 | 编译通过 |
| 5 | `src/app/app.c` 的 `App_RestoreFromTray` 中，SetForegroundWindow 后追加同样的 Load→改字段→Save 逻辑 | 恢复时持久化状态 | 编译通过 |
| 6 | `src/app/app.c` 的 `App_Init` 中，`App_ApplyShellChromeState()` 之后、`App_EnsureCaretVisible()` 之前插入读取 `presence_state` 并恢复窗口状态的逻辑——启动时恢复隐藏状态 | 启动后自动恢复上次状态 | 编译通过，手动验证 |
| 7 | 验证：隐藏到托盘 → 关闭程序 → 重启 → 窗口自动隐藏到托盘 | 恢复正确 | 手动验证 |
| 8 | 验证：前台显示状态 → 关闭程序 → 重启 → 窗口正常显示 | 恢复正确 | 手动验证 |
