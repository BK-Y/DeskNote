# PL-002: Markdown 解析与显示（总览）

> **状态：历史探索草案。** 当前仓库不按本文执行；若与现状冲突，请以当前代码、`README.md`、`docs\design\architecture.md` 以及 `.ai\` 目录中的现行计划为准。

## 概述

本计划针对“Markdown 解析与显示”模块（核心模块），把解析器、渲染器、编辑器集成与测试拆分为若干子计划文件，便于并行实现与评审。

解析引擎：采用 `md4c`（C 库）作为解析核心；在其之上实现 Parser Adapter → DocumentModel → Renderer 的管线。

输出目标（MVP）
- 使用 md4c 实现一个稳定的同步解析到 DocumentModel 的实现。
- 提供 HTML Preview（WebView）作为首要渲染后端。
- 实现异步解析框架（后台解析 + 取消机制）并在编辑器中完成端到端集成（snapshot → parseAsync → patch 应用）。

结构与文件
- 本计划分拆为多个子计划文件：
  - `PL-002a-parser.md` — 解析器适配与 DocumentModel（Parser Adapter）
  - `PL-002b-renderer.md` — 渲染器（HTML Preview / 原生渲染）
  - `PL-002c-integration.md` — 编辑器集成（异步解析、取消、patch）
  - `PL-002d-testing.md` — 性能测试、基准与 CI 集成

请参见各子计划文件以获取详细任务分解、估时与验收标准。

---

如果你同意，我将把以下四个子计划文件写入 `desknote/docs/plan/`（我会继续写入）。