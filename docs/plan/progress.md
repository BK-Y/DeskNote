# Plan 进度表（历史归档）

> **状态：历史文档。** 当前开发进度以 `.ai\plan.md` 和对应 phase 文件为准。

## 当前实际快照

| 项目 | 状态 | 说明 |
|---|---|---|
| 分层重构 | ✅ done | 已拆分为 `app / core / editor / platform / render / storage / ui` |
| 最小文本输入 | ✅ done | 已支持字符输入、回车、退格、左右移动 |
| 光标测量 | ✅ done | 已使用 DirectWrite 精确定位光标 |
| IME 跟随 | ✅ done | 系统光标、组合窗、候选窗已跟随编辑光标 |
| 本地持久化 | ✅ done | 已保存到 `%LOCALAPPDATA%\DeskNote\note.md` 与 `state.ini` |
| 点击定位光标 | ⏳ next | 这是当前下一阶段的主目标之一 |
| 最小垂直浏览 | ⏳ next | 与点击定位同属下一阶段 |
| Markdown 语义渲染 | 📝 later | 尚未重新接入当前主链 |

## 说明

这份旧进度表不再按早期 A/B/C 阶段继续维护。  
后续请查看 `.ai\phases\phase-5.md` 及其后续阶段文件。
