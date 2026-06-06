# Plan-11c-06 — 托盘隐藏/恢复

## 核心问题

App_HideToTray / App_RestoreFromTray 直接调 StateStore_Save 持久化 presence_state，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| A：隐藏到托盘 | `StateData state; StateStore_Load(&state); state.presence_state = SHELL_PRESENCE_HIDDEN_TO_TRAY; StateStore_Save(&state);` → `Config_Set(KEY_PRESENCE_STATE, SHELL_PRESENCE_HIDDEN_TO_TRAY);` | App_HideToTray 改用 Config_Set 持久化 presence_state | 贴边→隐藏到托盘 → state.ini 中 presence_state=1 |
| B：从托盘恢复 | `StateData state; StateStore_Load(&state); state.presence_state = SHELL_PRESENCE_VISIBLE_FRONT; StateStore_Save(&state);` → `Config_Set(KEY_PRESENCE_STATE, SHELL_PRESENCE_VISIBLE_FRONT);` | App_RestoreFromTray 改用 Config_Set 持久化 presence_state | 从托盘恢复 → state.ini 中 presence_state=0 |

## 主链路

```
隐藏到托盘 → App_HideToTray → Config_Set(KEY_PRESENCE_STATE, HIDDEN) → ShowWindow(SW_HIDE)
从托盘恢复 → App_RestoreFromTray → Config_Set(KEY_PRESENCE_STATE, VISIBLE) → ShowWindow(SW_SHOW)
```

## 验收标准

- [ ] Hide/Restore 中的 StateStore_Save 被 Config_Set 替换
- [ ] 隐藏→恢复后 presence_state 值正确

## 分层归属

本步骤属于 app 层，将 App_HideToTray / App_RestoreFromTray 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [human] 应用已启动，窗口为贴边模式

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 编译语法 | `gcc -fsyntax-only src/app/app.c` | 0 error |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 |
|------|---------|---------|
| M-1 | 1. 贴边→窗口关闭（WM_CLOSE 隐藏）<br>2. 检查 state.ini | 窗口隐藏，托盘图标可见，state.ini `presence_state=1` |
| M-2 | 1. 托盘图标左键恢复<br>2. 检查 state.ini | 窗口恢复显示，state.ini `presence_state=0` |

### GATE 7 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
