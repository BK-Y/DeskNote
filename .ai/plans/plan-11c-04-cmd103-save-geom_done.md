# Plan-11c-04 — cmd103 进入贴边前保存浮动几何

## 核心问题

cmd103 从浮动切贴边前，直接调 StateStore_Save 保存浮动几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| cmd103 保存浮动几何 | `StateData st; StateStore_Load(&st); st.last_floating_* = rect.*; StateStore_Save(&st);` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rect.*)` | cmd103 从浮动切贴边前改用 Config_Set 保存浮动几何 | 窗口浮动在某位置 → 切贴边 → 再退出贴边 → 窗口应能恢复到贴边前的位置 |

## 主链路

```
菜单→从浮动切贴边 → cmd103 → GetWindowRect → Config_Set(last_floating_*) → Config_Set(dock) → AppBar ops
```

## 验收标准

- [ ] cmd103 浮动几何保存中的 StateStore_Save 被 Config_Set 替换
- [ ] 切贴边后用 Config_Get 读到刚保存的浮动几何

## 分层归属

本步骤属于 app 层，将 cmd103 浮动几何保存部分的 StateStore 调用改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
