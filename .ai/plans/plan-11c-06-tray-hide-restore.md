# Plan-11c-06 — 托盘隐藏/恢复

## 目标
将 App_HideToTray / App_RestoreFromTray 中的 StateStore_Save 替换为 Config_Set。

## 前置
Config_Init 已调用。

## 步骤 A：隐藏到托盘

将 App_HideToTray 中的：

```c
StateData state;
StateStore_Load(&state);
state.presence_state = (int)SHELL_PRESENCE_HIDDEN_TO_TRAY;
StateStore_Save(&state);
```

改为：

```c
Config_Set(KEY_PRESENCE_STATE, (int)SHELL_PRESENCE_HIDDEN_TO_TRAY);
```

## 步骤 B：从托盘恢复

将 App_RestoreFromTray 中的：

```c
StateData state;
StateStore_Load(&state);
state.presence_state = (int)SHELL_PRESENCE_VISIBLE_FRONT;
StateStore_Save(&state);
```

改为：

```c
Config_Set(KEY_PRESENCE_STATE, (int)SHELL_PRESENCE_VISIBLE_FRONT);
```

## 验证

贴边→隐藏到托盘→从托盘恢复 → presence_state 值在 state.ini 中正确对应 hide/visible。

## 分层归属

本步骤属于 app 层，将 App_HideToTray / App_RestoreFromTray 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
