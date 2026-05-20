# desknote 文档规范与要求（README）

目的

本文件说明 `desknote/docs/` 下文档的组织、命名约定、管理规范以及协作流程，旨在保证文档一致性、便于查阅与长期维护。

目录结构（约定）

- `desknote/docs/notes/` — 学习笔记、临时记录、工具命令、实践心得（非严格评审）。
- `desknote/docs/plan/` — 规划与里程碑、任务分解、Sprint 文档（可映射到 issue）。
- `desknote/docs/adr/` — Architecture Decision Records（架构决策记录），每个 ADR 单独一个文件，按编号排序。

命名与格式约定

- 文档统一使用 Markdown 格式（`.md`）。
- ADR 文件命名：`NNNN-description.md`（例如 `0001-use-md4c.md`），文件开头包含元数据：日期、状态、决策者。
- Plan 项目：每个 plan 条目应带唯一 ID（如 `PL-001`）、title、design_refs、tasks、owner、estimate、status、acceptance_criteria。
- Notes：按主题命名（`git.md`、`md4c-integration.md`），内容可自由扩展。

Commit / PR / Branch 流程

- Commit message 推荐采用 Conventional Commits：
```desknote/docs/readme.md#L1-6
type(scope): short summary
```
- 分支策略：
  - `main`：发布用稳定分支
  - `develop`：日常集成
  - `feature/*`：新功能分支
  - `hotfix/*`：线上紧急修复
- PR 要求：
  - PR 标题/描述中写明关联 Plan ID（如 `PL-001`）与相关 ADR（如 `ADR-0001`），并关联对应 Issue。
  - 必须通过 CI，至少一位 Reviewer 才能合并。

ADR（架构决策记录）规范

- 每条 ADR 包含：Context（上下文）、Decision（决策）、Consequences（后果/权衡）、状态、日期、决策者。
- ADR 放在 `desknote/docs/adr/`，并在需要的 plan 中通过 `design_refs` 引用。

Plan 与 Design 的关联

- 所有 plan 条目必须包含 `design_refs` 字段，指向具体的 ADR 或 docs 文档（例如 `docs/adr/0001-use-md4c.md` 或 `docs/architecture.md#markdown-rendering`）。
- 在 issue / PR 中引用 Plan ID，确保实现对齐到计划与设计。

子模块与第三方依赖说明

- 若仓库使用 git submodule（例如 `lib/md4c`），请在 README/CI 中注明：
```desknote/docs/readme.md#L30-36
# 克隆并初始化子模块
git clone --recurse-submodules <repo>
# 或
git clone <repo>
git submodule update --init --recursive
```
- Plan 中若涉及第三方依赖升级，应记录在对应的 plan 项里并引用相关 ADR/notes。

CI 与文档校验

- CI 应保证：
  - 子模块会被初始化（`submodules: true` 或显式 `git submodule update --init --recursive`）。
  - 文档拼写/链接检查（可选）。
  - plan 相关自动检查（可选：例如 PR 必须包含 Plan ID）

贡献指南（简明）

- 新增/修改文档：在对应目录新增或修改文件，并在 PR 描述中说明变更目的与影响。若修改 ADR 或重要设计，请在 PR 中说明理由并 @ 项目维护者。 
- 若添加新的 Plan 项或变更现有 Plan：在 `desknote/docs/plan/` 新建/更新对应文件，并在 PR 中关联 issue。

示例模板（快速拷贝）

- ADR 模板文件示例：
```desknote/docs/adr/0000-template.md#L1-40
# ADR 0000 — Title

日期: YYYY-MM-DD
状态: Proposed / Accepted / Deprecated
决策者: <name>

Context
- 描述背景与问题。

Decision
- 描述选择的方案。

Consequences
- 列出优点与缺点。
```

- Plan 项目示例：
```desknote/docs/plan/PL-000-template.md#L1-40
# PL-000: Short title

- id: PL-000
- title: Short title
- design_refs: docs/adr/0000-template.md
- description: 描述目的与交付物
- tasks:
  - T1: 子任务说明 (owner)
- owner: person
- estimate: 2d
- status: todo
- acceptance_criteria:
  - 验收项 1
```

附记

- 本 README 作为项目文档规范的落地点，后续若需扩展可在对应子目录增加 `README.md` 进一步细化。

---

如果你同意，我可以：
- 将上述 `desknote/docs/readme.md` 文件写入仓库（我已完成），并可以继续根据你的指示在 `desknote/docs/` 下创建 ADR/plan/notes 模板或 README。