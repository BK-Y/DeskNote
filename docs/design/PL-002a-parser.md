# PL-002a: Parser Adapter 与 DocumentModel

> **状态：历史探索草案。** 当前仓库不按本文执行；若与现状冲突，请以当前代码、`README.md`、`docs\design\architecture.md` 以及 `.ai\` 目录中的现行计划为准。

## 目标
- 使用 `md4c` 将 Markdown 文本解析为内部 DocumentModel（节点流/AST），并提供同步与异步 API。
- DocumentModel 应支持后续的差分比较与 patch 生成（便于最小更新渲染）。

## 设计要点
- Parser Adapter（C 层封装）负责：
  - 配置 md4c flags（MD_FLAG_TABLES、MD_FLAG_STRIKETHROUGH 等）。
  - 注册 md4c 回调并把事件转换为内部节点（Block / Span / Text / Attr）。
  - 输出统一编码的文本（UTF-8），并对实体、空白、换行做规范化。
- DocumentModel 应满足：
  - 轻量：占用内存合理，适合快速 diff。
  - 表达力：能表达标题、段落、列表、表格、代码块、链接、图片、强调等。
  - 可 diff：支持按节点对比并生成可应用到渲染层的最小 patch。

## API 设计（草案）
- `int parser_parse(const char* text, size_t size, ParserConfig*, DocumentModel** out)` — 同步解析。返回 0 成功。
- `TaskHandle parser_parse_async(const char* text, ParserConfig*, callback, user)` — 异步接口，支持取消。
- DocumentModel 提供 `serialize()/deserialize()` 用于缓存与测试。

## 任务拆解与估时
- T1: 设计 DocumentModel 数据结构（1d）
- T2: 实现 md4c 回调到 DocumentModel 的同步实现（2d）
- T3: 实现异步解析框架（后台线程、取消 token、版本号）（1d）
- T4: 单元测试：覆盖常用语法与边界（1d）

估时合计：约 5 天

## 验收标准
- Parser 对常见 Markdown 语法的解析通过单元测试；
- 异步解析返回正确的 DocumentModel，且在高频输入（模拟快速键入）下能正确 cancel 过期任务；
- DocumentModel 能被序列化并恢复为相同结构（round-trip）。

## 风险与缓解
- md4c 回调细节复杂：编写覆盖实体、空白、换行的测试；严格按 md4c 文档实现映射。
- 事件到节点的映射可能造成细微差异（影响渲染一致性）：优先保证功能正确，再优化细节。