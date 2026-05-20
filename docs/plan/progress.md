# Plan 进度表（Progress Tracker）

目的：记录并追踪各个 Plan / 任务的当前状态，方便团队与个人查看进度、瓶颈与下步行动。

使用建议：
- 轻量首选：把本文件作为单一事实来源（single source of truth）。
- 每次任务状态变更时，更新此文件并提交（或者使用 issue/Project 自动化同步）。
- 对于需要更细粒度追踪的任务，建议同时创建 GitHub Issue 并在此表中填入 Issue 链接。

字段说明：
- ID：计划或任务的唯一标识（如 `PL-001` / `T1.1` / `PL-002a-T2`）。
- Title：简短标题。
- Owner：负责人。
- Status：状态（todo / in-progress / blocked / review / done）。
- Start：开始日期（可选）。
- Due：预期完成日期（可选）。
- Estimate：预计工时（可选）。
- Progress：完成百分比（0-100%）。
- Blockers：当前阻塞项（若无填 - ）。
- Notes：短备注或下一步动作。
- Design refs：相关设计文档或 ADR 路径。

---

## 总览表

| ID | Title | Owner | Status | Start | Due | Estimate | Progress | Blockers | Notes | Design refs |
|---:|:------|:------|:------:|:-----:|:---:|:--------:|:--------:|:--------:|:-----:|:-----------:|
| PL-001 | 核心数据模型与恢复 | peiyang | in-progress | 2026-05-20 | 2026-06-02 | 10.5d | 10% | - | M1 存储层开始 | docs/plan/PL-001-core.md |
| PL-001-T1.1 | note 存储目录与 I/O API | peiyang | in-progress | 2026-05-20 | 2026-05-21 | 2d | 20% | - | 设计并实现 API | docs/plan/PL-001-core.md |
| PL-001-T2.1 | journal 事件格式 | peiyang | todo | - | 2026-05-24 | 1.5d | 0% | - | 需要定义 schema | docs/architecture.md |
| PL-001-T8.1 | 集成 demo（smoke-test） | peiyang | todo | - | 2026-05-25 | 0.5d | 0% | - | demo 将验证恢复逻辑 | docs/plan/PL-001-core.md |
| PL-002 | Markdown 解析与显示（总览） | peiyang | todo | - | 2026-06-05 | 10d | 0% | - | 拆分 PL-002a..d | docs/plan/PL-002-markdown-renderer.md |
| PL-002a | Parser Adapter 与 DocumentModel | peiyang | in-progress | 2026-05-21 | 2026-05-26 | 5d | 10% | - | 实现同步 parse 与测试 | docs/plan/PL-002a-parser.md |
| PL-002b | Renderer（HTML Preview） | peiyang | todo | - | 2026-05-29 | 5.5d | 0% | 需决定 WebView 技术 | docs/plan/PL-002b-renderer.md |


---

## 使用与维护指南
- 更新频率：建议每日开发结束时更新本表（或任务完成时即时更新）。
- 责任：每项任务的 `Owner` 负责保持该行信息最新。若多人协作，Owner 可设为协作人。
- Blockers：在此列列出阻塞进度的问题并在 Notes 中写明需要谁来解决；方便在 daily/weekly review 中跟进。
- 追踪工具：若你使用 GitHub Issues / Projects，建议在 Issue 描述中写明 Plan ID，并把 Issue 链接填入 Notes 或在 Progress 表中添加 `#issue_number`。

---

## 快速操作（脚本/自动化建议）
- 可编写小脚本把 GitHub Issue 的状态同步到本表（或反向），也可用 GitHub Projects Board 做可视化进度追踪。

---

（你可以直接编辑此文件来更新进度，或者让我根据 issue/PR 自动化同步。需要我现在把当前计划的 tasks 全部展开并写入表格吗？）
