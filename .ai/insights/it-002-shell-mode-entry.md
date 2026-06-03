# it-002 — Shell_SetMode 唯一入口设计

## 问题
模式切换逻辑散落在 8 处代码，AppBar 注册/注销路径不统一。

## 方案
抽 Shell_SetMode 为唯一入口，冷启动和运行时都走它：
- 读当前模式 → 退旧模式（清理伴生行为）→ 写内存 → 入新模式（设置伴生行为）→ 持久化
- AppBar 调用只出现在 Shell_SetMode 内部

## 影响
- 删除 App_TryRegisterAppBarFromState（冷启动专用路径）
- 菜单 cmd103 也走 Shell_SetMode
- 修复 AppBar_SetPosition 中的 `SPI_GETWORKAREA` 改为 `MonitorFromWindow` + `GetMonitorInfo`

## 关联计划
`plans/plan-13-split-shell.md`

## 讨论记录
2026-06-03 与用户讨论确定方向。
