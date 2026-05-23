# PL-001: 核心数据模型与窗口行为实现计划（基于 ADR-0001）

## 概述

本计划基于 ADR-0001（采用 journal + checkpoint 恢复策略）与 `docs/architecture.md` 中的设计决策，目标是实现 desknote 的最小可用核心（MVP-1），包括笔记的文件化存储、append-only 日志、启动时恢复逻辑、多窗口行为、以及全局预览（Overview）。

设计参考（必读）
- docs/adr/0001-journal-recovery.md
- docs/architecture.md

## 目标
- 实现以 Markdown 文件为最终存储的笔记模型（每条 note 为文件夹，包含 note.md, state.json, journal.log）。
- 保证应用崩溃/断电场景下的快速恢复（通过 journal replay）。
- 支持多窗口：新增窗口创建新 note；多个窗口对同一 note 的编辑通过 journal 顺序回放合并。
- 提供全局预览窗口，基于 index.json 展示摘要、支持搜索/筛选，并能在当前或新窗口打开笔记。

## 范围（MVP-1）
- Note 存储结构与加载/保存 API
- Append-only journal 写入格式与实现
- 启动时的 replay 恢复逻辑
- Periodic checkpoint（可配置，默认 5s）与 journal compaction（基本实现）
- 关闭时的原子保存流程和简单冲突提示（LWW + 备份）
- 新窗口语义（创建新 note）与启动时默认打开上次活动 note
- 全局预览窗口（Overview）基础功能：列表、摘要、打开（不包含高级搜索索引）

## 里程碑与任务分解

- M1: 存储层与 API（2d）
  - T1.1: 设计并实现 note 目录结构与 I/O API（load/save/checkpoint）
  - T1.2: 实现 index.json 的读写与原子更新工具（tmp + rename）

- M2: Journal（1.5d）
  - T2.1: 定义 journal 事件格式（timestamp, window-id, op-type, payload）
  - T2.2: 实现 append-only 写入接口（支持 fsync/flush 可选）

- M3: 恢复与合并（1.5d）
  - T3.1: 实现启动时 replay 逻辑（从 note.md + journal.log 恢复工作缓冲）
  - T3.2: 多窗口基本合并策略（按时间戳 replay；结构性变更触发确认）

- M4: Checkpoint 与 Compaction（1d）
  - T4.1: 实现 periodic checkpoint（默认 5s，可配置）
  - T4.2: 实现 journal compaction（将已合并事件截断或归档）

- M5: 窗口行为与默认打开（1d）
  - T5.1: 实现新增窗口：创建 note、初始化 state.json、在 index.json 注册
  - T5.2: 实现启动时默认打开最后活动 note（记录/读取 global_state）

- M6: 全局预览窗口（Overview）（1.5d）
  - T6.1: 实现基于 index.json 的笔记列表与摘要展示
  - T6.2: 支持按标签/修改时间简单筛选与在当前/新窗口打开笔记

- M7: 关闭流程与冲突提示（1d）
  - T7.1: 实现关闭时的原子保存流程（暂停输入 → checkpoint → update index → resume）
  - T7.2: 简单冲突提示 UI（内存 vs 磁盘 vs 合并 vs 保存为新文件）

- M8: 集成与 demo（0.5d）
  - T8.1: 集成到顶层 CMake，添加 smoke-test 程序（创建 note、写 journal、重启后验证恢复）


## 时间估算
总估时：约 10.5 天（可并行任务可压缩为一周内完成小组协作）。

## 验收标准（Acceptance Criteria）
- 在本地创建并编辑 note，触发若干 journal 事件后，重启应用，note 能通过 replay 恢复到最近输入状态。
- 新增窗口时创建新的 note 目录并在 index.json 中注册；默认启动打开最后活动 note。
- Overview 能展示笔记列表并打开选中笔记。
- 关闭时走原子保存流程；当检测到磁盘已有更新导致不一致时，能弹窗提示并允许用户选择保留策略。

## 风险与缓解
- 风险：journal 未设计为 idempotent 导致重复 replay 出错。缓解：设计事件幂等性并在 replay 过程中检测并忽略已应用的事件（事件带序号/UUID）。
- 风险：长期未 compaction 导致日志膨胀。缓解：实现定期 compaction 与可手动触发的清理工具。
- 风险：并发结构性变更复杂冲突。缓解：在首次实现采用用户确认策略，长期考虑 CRDT。

## 依赖
- lib/md4c（submodule）—— 用于 Markdown 渲染 / demo 验证（非核心数据逻辑，但在集成 demo 中使用）。

## 后续（MVP 后）
- 引入 SQLite 索引以支持快速搜索/筛选（可选模块）。
- 改进并发合并策略（CRDT 或 OT）。
- 同步服务原型（server + client），实现云端备份/同步。


---

如果你确认，我将：
1. 把本计划保存为 `desknote/docs/plan/PL-001-core.md`（已完成）；
2. 基于任务清单为每个任务生成对应的 issue 模板（可选）；
3. 开始实现 M1 的存储层 demo 并提交 PR（需你授权我在仓库内创建实现代码）。
