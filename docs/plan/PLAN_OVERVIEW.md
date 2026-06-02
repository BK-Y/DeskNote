# DeskNote 开发计划总览（历史归档）

> **状态：历史文档。**  
> 当前执行计划已经迁移到 `.ai\plan.md`、`.ai\phases\`、`.ai\standards\phase-planning.md`。  
> 本文件只保留为早期计划思路的归档入口。

## 当前实际进度

当前代码已经完成：

- 分层重构：`app / platform / render / ui / editor / core / storage`
- 最小文本输入链：字符输入、回车、退格、左右移动
- DirectWrite 精确光标测量
- 系统光标与 IME 跟随
- `%LOCALAPPDATA%\DeskNote\note.md` 与 `state.ini` 持久化

当前下一个实现阶段是：

- **Phase 5：点击定位光标 + 最小垂直浏览 + 光标/视口耦合**

## 当前源码结构（已更新）

```text
src/
├── app/
├── core/
├── editor/
├── platform/win32/
├── render/
├── storage/
└── ui/
```

## 本目录中文件的意义

- `PLAN_STAGE_0-1.md`：早期窗口 / 基础阶段记录
- `PLAN_STAGE_2-4.md`：早期编辑 / 存储阶段记录
- `PLAN_STAGE_5-11.md`：更早版本的远期设想
- `progress.md`：旧版进度表

这些文件**不是当前任务拆解来源**。如果需要继续开发，请直接查看 `.ai\` 目录中的现行计划。
