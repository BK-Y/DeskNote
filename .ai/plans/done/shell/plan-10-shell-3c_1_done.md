# Shell-3c_1 — 关闭策略与退出入口重新定义（后续）

## ① 核心问题

现在所有"关闭"都是真的退出程序。但你要求 Shell-3 完成后的行为是：
- 标题栏汉堡菜单的"关闭" → 隐藏到托盘（不退出）
- 托盘右键菜单的"退出" → 唯一退出入口
- 窗口关闭按钮（×）→ 隐藏到托盘

当前 3c_1 的规划还是按旧的"标题栏菜单关闭→退出"来写的，与新需求冲突。

## ② 分析现状

### 阶段摘要

重新定义关闭和退出的行为：
- 隐藏（hide）：窗口隐藏到托盘，不退出 → 触发场景：窗口 ×、Alt+F4、标题栏菜单"关闭"
- 退出（exit）：真正终止程序 → 触发场景：**唯一入口**托盘右键菜单"退出"

### 承接关系

前置阶段：Shell-3b（presence 状态管理和显隐函数 App_HideToTray / App_RestoreFromTray 已就绪）
前置阶段：Shell-3a_2（托盘菜单已创建，含"退出"项 command 1002）

## ③ 方案设计

候选方案：
1. **标题栏菜单"关闭"改为调用 App_HideToTray** — 修改 App_SubmitShellCommand 中 cmd==100 的分支
2. **保留标题栏菜单"关闭"为退出，但增加确认弹窗** — 不符合需求

选择方案 1 [selected]。理由：直接改弹窗菜单的命令分发逻辑，改动最小。

## ④ 拆解执行

### 整体目标

1. **标题栏汉堡菜单"关闭"** → 隐藏到托盘，不退出
2. **窗口 × / Alt+F4** → 隐藏到托盘，不退出
3. **标题栏汉堡菜单"关于"** → 保持不变
4. **托盘右键菜单"退出"** → 唯一退出入口，终止程序
5. 移除代码中所有其他退出路径

### 阶段产出

1. 点标题栏汉堡菜单"关闭" → 窗口隐藏到托盘
2. 点窗口 × → 窗口隐藏到托盘
3. 点托盘右键菜单"退出" → 程序终止
4. 没有其他方式能退出程序

## ⑤ 设定边界

### 本阶段范围

只做关闭/退出命令的分发重定义。

### 不包括

1. 不做状态持久化（Shell-3c_2）
2. 不做 presence 状态转换逻辑（已经在 Shell-3b 完成）
3. 不做托盘菜单创建（已经在 Shell-3a_2 完成）

### 分层归属

- **app/** — App_SubmitShellCommand 中 cmd==100 分支从 PostQuitMessage(0) 改为 App_HideToTray
- **platform/win32/window** — WM_CLOSE 拦截调 App_HideToTray；WM_COMMAND 1002（托盘退出）仍为 DestroyWindow（唯一退出入口）

### 文件落点

#### 预计修改文件

- `src/app/app.c` — cmd==100 改为调 App_HideToTray，而非 PostQuitMessage
- `src/platform/win32/window.c` — 新增 WM_CLOSE 拦截，调 App_HideToTray

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`

## ⑥ 落地方案

### 技术路线

**app.c（标题栏菜单"关闭"命令分发）：**

```c
if (cmd == 100) /* 关闭 → 改为隐藏到托盘 */
    App_HideToTray(g_app.hwnd);
else if (cmd == 101) /* 关于 */
    MessageBoxW(...);
```

**几点说明：**
- cmd==100 从 PostQuitMessage(0) 改为 App_HideToTray
- cmd==101（关于）保持不变
- 之前旧的"标题栏菜单退出"意图全部移除
- 退出仅保留托盘菜单"退出"(1002) → 已在 Shell-3a_2 中定义为 DestroyWindow，作为唯一退出入口

### 主链路

```text
用户点标题栏汉堡菜单"关闭"
-> App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_MENU)
-> cmd == 100
-> App_HideToTray(hwnd) → ShowWindow(SW_HIDE) → 窗口隐藏到托盘

用户点窗口 ×
-> WM_CLOSE
-> App_HideToTray(hwnd) → ShowWindow(SW_HIDE) → 窗口隐藏到托盘

用户点托盘右键菜单"退出"
-> WM_COMMAND 1002
-> DestroyWindow(hwnd) → 程序退出
```

## ⑦ 验收标准

1. 标题栏汉堡菜单"关闭" → 隐藏到托盘，不退出
2. 窗口关闭按钮（×）→ 隐藏到托盘，不退出
3. Alt+F4 → 隐藏到托盘，不退出
4. 标题栏汉堡菜单"关于" → 正常弹出关于对话框
5. 托盘右键菜单"退出" → 程序终止（唯一退出入口）
6. 分层归属与文件落点一致

## ⑧ 推进步骤

### 方案 1：标题栏菜单"关闭"改为调用 App_HideToTray [selected]

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/app/app.c` 中 `App_SubmitShellCommand` 的 `cmd == 100` 分支，将 `PostQuitMessage(0)` 改为 `App_HideToTray(g_app.hwnd)`——所有关闭操作统一走 App_HideToTray，不直接退出 | 标题栏菜单"关闭"不再退出，改为隐藏 | 编译通过 |
| 2 | `src/platform/win32/window.c` 的 WndProc 中新增 `case WM_CLOSE:`，调用 `App_HideToTray(hwnd)`——拦截窗口关闭按钮和 Alt+F4，不退出 | 窗口 × 和 Alt+F4 走隐藏逻辑 | 编译通过 |
| 3 | 验证：点标题栏汉堡菜单"关闭"，窗口隐藏到托盘不退出 | 关闭行为正确 | 手动验证 |
| 4 | 验证：点窗口 ×，窗口隐藏到托盘不退出 | 关闭行为正确 | 手动验证 |
| 5 | 验证：Alt+F4，窗口隐藏到托盘不退出 | 关闭行为正确 | 手动验证 |
| 6 | 验证：点托盘右键菜单"退出"，程序终止（唯一退出入口） | 退出入口正确 | 手动验证 |
