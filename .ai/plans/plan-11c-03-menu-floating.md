# Plan-11c-03 — 菜单浮动置顶

## 核心问题

菜单 cmd102 直接调 StateStore_Load/Save 保存窗口几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| cmd102 保存浮动几何 | `StateData state; StateStore_Load(&state); state.last_floating_* = rect.*; StateStore_Save(&state);` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rect.*)` | cmd102 浮动置顶路径改用 Config_Set 持久化窗口几何 | 移动窗口位置 → 菜单→浮动置顶 → 再点取消 → 窗口回到之前记录的坐标 |

## 主链路

```
菜单→浮动置顶 → cmd102 → GetWindowRect → Config_Set(last_floating_*) → SetWindowPos(HWND_TOPMOST)
```

## 验收标准

- [ ] cmd102 中的 StateStore 调用被 Config_Set 替换
- [ ] 窗口几何在切浮动模式后被持久化到 state.ini

## 分层归属

本步骤属于 app 层，将 cmd102 handler 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
