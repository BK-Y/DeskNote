# Plan-11d — 回调注册与收尾

## 核心问题

AppBar ops 散落在 11c 的 cmd103/OnEndDrag/启动路径中，未统一到 on_change 触发。`App_WriteShellStateData` 仍在写壳层字段到 StateData。

## 分析现状

- 前置：11c 已完成，app.c 中 9 处写入点全部替换为 Config_Set（仅文档管理 3 处保留 StateStore）
- 现状：cmd103 进入退出贴边仍保留显式 AppBar_Register/Unregister 调用，和 Config_Set 触发 on_change 后重复执行
- 产出：on_change 回调接管所有 AppBar ops，残留显式 AppBar 调用被删除，冗余函数被清理

## 方案选型

- **方案 A（选中）：在 app.c 中注册 on_change 回调，当前 app 层直接处理 AppBar ops**
  - 在 App_Init 中调用 Config_OnChange(OnShellModeChanged)
  - 删除 11c 迁移各步骤中残留的显式 AppBar_Register/Unregister
  - 符合当前架构：shell 拆分前 AppBar ops 由 app 层管理是现行事实

- **方案 B（排除）：等 shell 拆分后再注册 on_change**
  - 不修复双注册问题，保留 11c-02/05 的冗余 AppBar 调用
  - 排除理由：长期存在双注册风险，AppBar_Register 重复调用可能产生不可预测的系统行为

## 拆解执行

### 主链路

```
Config_Set("shell_resident_mode", EDGE_RESERVED)
  ├─ 更新内存表
  ├─ ini2arr_write → .tmp → rename → state.ini
  └─ on_change → OnShellModeChanged
       ├─ AppBar_Register + AppBar_SetPosition
       └─ SetWindowPos(HWND_NOTOPMOST)  /* exit old mode */

Config_Set("shell_resident_mode", NONE)
  └─ on_change → AppBar_Unregister
```

## 设定边界

### 范围

- 注册 on_change 回调使 Config_Set 自动触发 AppBar ops
- 删除 11c 迁移步骤中残留的显式 AppBar_Register/Unregister 调用
- 清理 App_WriteShellStateData 在 App_EnableCustomChrome 中的调用
- 删除 g_app.shell.resident_mode 的直接写入（已由 Config_Set 接管）

### 不做

- 不动 state_store.c / state_store.h
- 不动 App_SaveCurrentDocument 中的文档字段 StateStore_Save
- 不删除 App_TryRegisterAppBarFromState（11c-01 已简化，待 shell 拆分后移入 shell 模块）

### 不修改的文件

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*

## 落地方案

### on_change 回调函数

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
        int edge = Config_Get("dock_edge", APP_DOCK_RIGHT);
        int thick = Config_Get("dock_thickness", 240);
        AppBar_Register(hwnd);
        AppBar_SetPosition(hwnd, (AppDockEdge)edge, thick);
    }
    if (new_val == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
}
```

在 `App_Init` 中（`AppBar_TryRegisterAppBarFromState(hwnd)` 之前）注册：

```c
Config_OnChange(OnShellModeChanged);
```

## 验收标准

- [ ] `Config_Set("shell_resident_mode", EDGE_RESERVED)` 自动触发 AppBar_Register + AppBar_SetPosition
- [ ] `Config_Set("shell_resident_mode", NONE)` 自动触发 AppBar_Unregister
- [ ] cmd103/OnEndDrag 中不再有显式 AppBar_Register/Unregister（已由 on_change 接管）
- [ ] `App_WriteShellStateData` 在 App_EnableCustomChrome 中不再调用（但函数体保留，待后续）
- [ ] 分层核实：on_change 回调位于 app 层，暂不移动（shell 拆分后移入 shell 模块）
- [ ] Config_OnChange 在 App_Init 中仅注册一次，不存在重复注册导致回调被触发多次

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [agent] 所有 11c 子阶段已完成

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 编译语法 | `gcc -fsyntax-only src/app/app.c` | 0 error |
| A-2 | Config_OnChange 注册次数 | `grep -c 'Config_OnChange' src/app/app.c` | 1 行（仅 App_Init 中注册一次） |
| A-3 | AppBar_Register 残留 | `grep -c 'AppBar_Register' src/app/app.c` | ≤1 行（仅 on_change 回调中保留） |
| A-4 | AppBar_Unregister 残留 | `grep -c 'AppBar_Unregister' src/app/app.c` | ≤1 行（仅 on_change 回调中保留） |
| A-5 | App_WriteShellStateData 调用 | `grep -c 'App_WriteShellStateData' src/app/app.c` | ≤1 行（仅文档管理） |
| A-6 | StateStore 调用残留 | `grep 'StateStore' src/app/app.c` | 仅剩文档管理相关调用 |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 |
|------|---------|---------|
| M-1 | 1. 菜单→贴边占位 | Config_Set 触发 on_change → AppBar_Register + SetPosition，无重复注册 |
| M-2 | 1. 贴边→退出贴边 | Config_Set 触发 on_change → AppBar_Unregister |
| M-3 | 1. 贴边→拖离浮动 | Config_Set 触发 on_change → AppBar_Unregister + SetWindowPos(HWND_TOPMOST) |

### GATE 8 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈

## 推进步骤

### 方案 A：在 app.c 中注册 on_change 回调 [selected]

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | 在 app.c 的 App_Init 中，App_TryRegisterAppBarFromState 之前调用 `Config_OnChange(OnShellModeChanged)` | config callback 注册完成 | `gcc -fsyntax-only` 通过 |
| 2 | 在 app.c 中添加 `OnShellModeChanged` 回调函数定义 | 回调函数存在 | 同上 |
| 3 | 删除 cmd103 进入贴边分支中的显式 `AppBar_Register + AppBar_SetPosition` | 进入贴边只调 Config_Set，on_change 自动触发 AppBar 注册 | `git diff` 确认删除 |
| 4 | 删除 cmd103 退出贴边分支中的显式 `AppBar_Unregister` | 退出贴边只调 Config_Set，on_change 自动触发 AppBar 注销 | `git diff` 确认删除 |
| 5 | 删除 App_OnEndDrag mode=1 中的显式 `AppBar_Unregister` | 同理由 on_change 接管 | `git diff` 确认删除 |
| 6 | 删除 App_OnEndDrag mode=2 中的显式 `AppBar_Unregister` | 同上 | `git diff` 确认删除 |
| 7 | 在 App_EnableCustomChrome 中移除 App_WriteShellStateData 调用 | App_WriteShellStateData 不再通过 App_EnableCustomChrome 写入壳层字段 | `grep "App_WriteShellStateData" src/app/app.c` 仅剩文档管理 1 处 |
| 8 | 验证：全部 6 项回归测试 | 冷启动贴边/浮动、菜单切贴边/浮动、拖拽、托盘 | 见验证表 |

### 方案 B：等 shell 拆分（备选）

（本方案未选中，推进步骤为空）
