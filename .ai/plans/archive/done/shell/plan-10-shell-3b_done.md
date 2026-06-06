# Shell-3b — 任务栏/托盘 presence 矩阵与显隐链（后续）

## ① 核心问题

窗口只有"显示"和"关闭"两种状态，没有"隐藏到托盘但不退出"的中间态。ShowWindow(SW_HIDE) 隐藏后任务栏仍然占位。

## ② 分析现状

### 阶段摘要

从"仅有显示/关闭二态"推进到"定义 visible_front / hidden_to_tray / exiting 三种状态，并实现状态间的转换规则"。

### 承接关系

前置阶段：Shell-3a_2（托盘消息和菜单已就绪，菜单项可触发命令）

## ③ 方案设计

候选方案：
1. **app 层维护 ShellPresenceState 枚举 + platform 层按状态调用 ShowWindow** — 职责清晰，app 决策 platform 执行
2. **platform 层自己维护状态** — 违反"关闭策略的业务判断留在 app"的架构约束

选择方案 1。理由：符合架构文档中"app 负责收束壳层模式切换"的约定。

## ④ 拆解执行

### 整体目标

1. 定义 ShellPresenceState 枚举：visible_front / hidden_to_tray / exiting
2. 实现 visible_front → hidden_to_tray 转换（隐藏窗口、释放任务栏占位）
3. 实现 hidden_to_tray → visible_front 转换（恢复窗口、恢复任务栏占位）
4. 确保 hidden_to_tray 时任务栏图标消失

### 阶段产出

1. app 中可查询当前窗口 presence 状态
2. 调用隐藏 → 窗口隐藏且任务栏图标消失
3. 调用显示 → 窗口恢复前台，任务栏正常显示

## ⑤ 设定边界

### 本阶段范围

只做 presence 矩阵的状态定义和转换。

### 不包括

1. 不做关闭策略细则（Shell-3c_1）
2. 不做状态持久化（Shell-3c_2）
3. 不做托盘菜单改造

### 分层归属

- **app/** — 定义 ShellPresenceState 枚举，实现状态转换逻辑（App_HideToTray / App_RestoreFromTray）
- **platform/win32/window** — 提供 ShowWindow 配合，接收 app 的指令执行窗口显隐

### 文件落点

#### 预计修改文件

- `src/app/app.h` — 新增 ShellPresenceState 枚举、App_HideToTray / App_RestoreFromTray 声明
- `src/app/app.c` — 实现 presence 状态转换逻辑
- `src/platform/win32/window.c` — ShowWindow 配合 presence 切换

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`

## ⑥ 落地方案

### 技术路线

**app.h：**

```c
typedef enum {
    SHELL_PRESENCE_VISIBLE_FRONT,
    SHELL_PRESENCE_HIDDEN_TO_TRAY,
    SHELL_PRESENCE_EXITING
} ShellPresenceState;

// 获取当前 presence 状态
ShellPresenceState App_GetPresenceState(void);

// 隐藏到托盘（visible_front → hidden_to_tray）
void App_HideToTray(HWND hwnd);

// 从托盘恢复前台（hidden_to_tray → visible_front）
void App_RestoreFromTray(HWND hwnd);
```

**app.c：**

```c
static ShellPresenceState s_presence_state = SHELL_PRESENCE_VISIBLE_FRONT;

ShellPresenceState App_GetPresenceState(void)
{
    return s_presence_state;
}

void App_HideToTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_HIDDEN_TO_TRAY;
    ShowWindow(hwnd, SW_HIDE);
    /* hidden_to_tray 时任务栏图标消失 */
    /* 注意：SW_HIDE 已自动处理任务栏占位释放 */
}

void App_RestoreFromTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_VISIBLE_FRONT;
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    /* 恢复前台时任务栏正常显示 */
}
```

### 主链路

```text
用户选择"隐藏到托盘"
-> app 调用 App_HideToTray(hwnd)
-> s_presence_state = HIDDEN_TO_TRAY
-> ShowWindow(SW_HIDE)
-> 窗口隐藏，任务栏图标消失

用户左键/菜单"显示窗口"
-> app 调用 App_RestoreFromTray(hwnd)
-> s_presence_state = VISIBLE_FRONT
-> ShowWindow(SW_SHOW) + SetForegroundWindow
-> 窗口恢复前台，任务栏正常显示
```

### 接口说明

```c
// 隐藏到托盘：visible_front → hidden_to_tray
void App_HideToTray(HWND hwnd);

// 从托盘恢复前台：hidden_to_tray → visible_front
void App_RestoreFromTray(HWND hwnd);

// 获取当前 presence 状态
ShellPresenceState App_GetPresenceState(void);
```

调用顺序：
- 隐藏：App_GetPresenceState() → 确认状态合法 → App_HideToTray(hwnd)
- 恢复：App_GetPresenceState() → 确认状态合法 → App_RestoreFromTray(hwnd)

## ⑦ 验收标准

1. hidden_to_tray 时窗口隐藏且不在任务栏占位
2. visible_front 时窗口正常显示在任务栏
3. 转换过程无闪烁或无窗口消失不可恢复
4. 分层归属与文件落点一致

## ⑧ 推进步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | `src/app/app.h` 新增 ShellPresenceState 枚举 + App_HideToTray/App_RestoreFromTray/App_GetPresenceState 声明 | 类型和函数声明就绪 | 编译通过 |
| 2 | `src/app/app.c` 新增静态变量 `s_presence_state` + 三个函数实现 | presence 转换逻辑就绪 | 编译通过 |
| 3 | `src/platform/win32/window.c` 中将托盘菜单"退出"的 ShowWindow/SW_HIDE 调用替换为 App_HideToTray(hwnd) | 隐藏操作走统一的状态管理 | 编译通过 |
| 4 | `src/platform/win32/window.c` 中将 WM_COMMAND "显示窗口"的 ShowWindow 替换为 App_RestoreFromTray(hwnd) | 恢复操作走统一的状态管理 | 编译通过 |
| 5 | 验证：让窗口隐藏到托盘，检查任务栏图标消失 | hidden_to_tray 行为正确 | 手动验证 |
| 6 | 验证：从托盘恢复窗口，检查任务栏图标恢复 | visible_front 行为正确 | 手动验证 |
