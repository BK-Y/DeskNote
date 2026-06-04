# Plan-11c-03 — 菜单浮动置顶

## 目标
将 cmd102 中的 StateStore_Save 替换为 Config_Set。

## 前置
Config_Init 已调用。

## 步骤

原代码（app.c L1415-1426）：

```c
StateData state; StateStore_Load(&state);
state.last_floating_left   = rect.left;
state.last_floating_top    = rect.top;
state.last_floating_width  = rect.right - rect.left;
state.last_floating_height = rect.bottom - rect.top;
StateStore_Save(&state);
```

改为：

```c
Config_Set(KEY_FLOATING_LEFT, rect.left);
Config_Set(KEY_FLOATING_TOP, rect.top);
Config_Set(KEY_FLOATING_WIDTH, rect.right - rect.left);
Config_Set(KEY_FLOATING_HEIGHT, rect.bottom - rect.top);
```

## 验证

移动窗口位置 → 菜单→浮动置顶 → 再点取消 → 窗口回到之前记录的坐标。

## 分层归属

本步骤属于 app 层，将 cmd102 handler 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
