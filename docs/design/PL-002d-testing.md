# PL-002d: 测试、性能基准与 CI 集成

## 目标
- 为 Markdown 解析与渲染模块建立自动化测试、性能基准与 CI 验收标准，确保解析正确性与性能回归可控。

## 测试类型
- 单元测试：Parser（md4c -> DocumentModel）和小块文法测试（headers, lists, tables, fenced code, entities）。
- 集成测试：编辑器 snapshot → parseAsync → patch 应用 的端到端流程。
- 性能测试：解析时长、diff/patch 计算时间、渲染更新耗时、内存占用（针对不同文档大小）。
- 恢复测试：journal replay 在不同日志场景下能恢复到预期状态。

## 基准用例
- 小文档：< 10 KB（短笔记）
- 中等文档：10 KB - 100 KB（常见）
- 大文档：0.5 MB - 5 MB（压力测试）

对每个用例记录：解析时间（平均/95pct）、patch 生成时间、内存峰值、渲染更新时间。

## CI 集成
- 在 CI 中运行单元测试与集成测试。性能基准可做 nightly/run-on-demand；若需，可把关键性能阈值作为 gate（例如解析 100 KB 的文档 < 200 ms），但初期建议只记录基准以避免频繁失败。
- 建议使用 small runner（如 Github Actions）执行：
  - unit tests (fast)
  - integration demo (parse+replay smoke)
  - optional: performance benchmark workflow（nightly）

## 任务拆解与估时
- T1: 单元测试用例编写与 CI hook（1d）
- T2: 集成测试脚本与 demo（0.5d）
- T3: 基准脚本（解析/patch/渲染计时）与样本数据生成（1d）
- T4: CI workflow 配置（测试 + nightly 基准）（0.5d）

估时合计：约 3 天

## 验收标准
- 单元测试覆盖主要语法场景且通过；
- 集成测试能够自动完成 parse→patch→render 的 demo 流程；
- 性能基准脚本能生成报告并保存到 CI artifacts（nightly）。
