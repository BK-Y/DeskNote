# desknote 架构设计文档

## 概要

核心方案：每个窗口维护三类状态并确保一致性 ——

1. 追加式操作日志（journal.log）—— 用户操作以追加的形式记录，用于快速持久化与恢复；
2. 内存中的 Markdown 文本（working buffer）—— 编辑器的即时文本状态，用于渲染与交互；
3. 文件结构树 / 索引（index.json / state.json）—— 管理笔记的目录结构、元信息与全局视图。

每个窗口都维护以上三类内容的会话级状态（含窗口 id），用户主动关闭时需要把三者合并并持久化；异常退出时通过 replay journal 恢复最新用户输入。

---

## 数据模型与存储布局

建议的本地数据目录（按平台）：

- Windows: `%APPDATA%\desknote\` 
- Linux: `~/.local/share/desknote/`

单个笔记（note）目录建议结构：

```
notes/
  <note-id>/
    note.md         # 最近 checkpoint（用于快速加载）
    state.json      # 元数据：title, tags, last_modified, version
    journal.log     # append-only 日志（按顺序记录用户操作事件）
```

全局索引：

```
index.json         # 全局文件/笔记树结构与快捷元数据（id, parent, order）
```

---

## 三类数据与一致性策略

### 追加日志（journal.log）
- 以 append-only 方式记录用户每次“操作事件”：timestamp、op-type、window-id、payload（可为最小变更描述或操作序列）。
- 快速写入，便于崩溃恢复与审计。

### 内存 buffer（working buffer）
- 编辑器持有的即时文本状态，用于渲染与交互。
- 通过定期 checkpoint（例如每 N 秒或在关键操作后）把当前 buffer 合并到 `note.md`，同时将对应的 journal entries 标记为已合并或截断（compaction）。

### 文件结构树（index.json / state.json）
- 管理笔记的层级、排序、标签信息、以及结构性变更（移动/重命名/创建/删除）。
- 结构变更会产生 journal 事件并触发回调链路以保证 UI 与索引一致。

### 关闭时的保存流程（用户主动关闭）
1. 暂停编辑输入（短阻塞），防止并发写入。
2. 把 working buffer 做快照写入临时文件（atomic tmp）。
3. 将临时文件原子重命名为 `note.md`（保证原子性）。
4. 标记并 compact 已合并的 journal entries（可异步执行）。
5. 更新 `state.json` 与 `index.json`（如果有结构变更）。
6. 恢复 UI 操作。

若发现三者冲突（例如磁盘上已有更晚的变更），默认策略是弹窗由用户选择：保留内存/保留磁盘/尝试合并/另存为新版本；在简化实现中可采用 LWW（最后写入者胜出）并备份冲突版本到 `conflicts/`。

---

## 崩溃恢复（sudden crash）

启动时对每个 note 执行：
1. 读取 `note.md`（最后 checkpoint）。
2. 顺序 replay `journal.log` 中未合并的记录到 working buffer，恢复最近状态。可选地恢复光标/选择等 session 元数据。 
3. 若 index 与 state 存在不一致，根据 journal 事件修复索引。

日志可保证操作可重放（idempotent 的事件结构有利于恢复）。

---

## 多窗口与并发策略

- 每个窗口分配 `window-id` 并在 journal 中记录来源。
- 同一 note 在多个窗口同时编辑：journal 按时间戳顺序 replay（乐观顺序）。
- 结构性变更（如移动/删除/标签变更）需触发同步回调，并在必要时要求用户确认以避免未预期的冲突。
- 同一进程内的多窗口可以通过内存事件总线进行实时广播，减少磁盘写频率；跨进程多实例访问需额外的锁或单一守护进程来避免竞态。

---

## 输入分类与回调模型

### 输入分类
- 结构性输入（structure ops）：标签、移动、重命名、创建/删除文件、动作命令 —— 这些会改变文件结构与索引。
- 文本内容输入（text ops）：普通 Markdown 文本的插入/删除/替换 —— 只影响 note.md 与 journal。

### 回调模型
- 对结构性输入，触发回调链：更新 `index.json` → 更新 UI 列表 → 写 journal → 若需要立即持久化则 checkpoint。
- 对文本输入，append journal + 更新 working buffer + 局部渲染回调（避免全量渲染导致卡顿）。

回调应设计为轻量可取消的操作，保证用户输入路径的低延迟。

---

## 原子写入与数据安全

- 写入文件时先写临时文件并 fsync，然后用原子 rename 替换目标文件（避免中途损坏）。
- journal 写入建议至少在关键节点执行 fsync（或在 Windows 使用相应的 FlushFileBuffers）。
- 定期执行 journal compaction：把已 replay 的事件合并进 checkpoint 并截断旧日志。

---

## 平台优先级

1. Windows（首要）—— 优化 AppData 存储、Windows-specific fsync/rename 行为。
2. Linux（次要）—— 使用 POSIX 原语。
3. macOS（暂不考虑，目前不做适配）。

---

## 后续同步服务（Roadmap）

- 短期（MVP 后）支持两种同步模式：
  - 托管付费同步服务（中心化 server + auth）。
  - 用户自建同步服务器（可用简易 HTTP API / WebDAV / 自定义服务）。

- 冲突合并策略：短期采用 timestamp+LWW + manual conflict resolution；长期考虑引入 CRDT/OT 以实现更可靠的自动合并。
- 数据隐私：传输层使用 TLS；长期支持端到端加密（E2EE）及本地密钥管理。

---

## MVP 列表（优先级）

- MVP-1 (核心)
  - note 数据模型（note.md, state.json, journal.log）与 index.json。
  - append-only journal 写入与启动时 replay 恢复。
  - window-level working buffer 与 periodic checkpoint。
  - 原子写入与简单冲突提示（LWW + 备份）。
  - 单进程多窗口事件总线同步。

- MVP-2
  - metadata（tags, pinned）与结构操作回调。
  - 冲突 UI 与日志压缩（compaction）。

- MVP-3
  - 跨平台细节调整与打包（Windows 优化，Linux 支持）。

- MVP-4
  - 同步服务原型（server + client），初步冲突处理。

---

## 用户数据与窗口行为

本节定义用户数据与窗口层面的具体行为要求，覆盖最终存储、窗口打开/创建逻辑与预览窗口的设计。

### 最终存储格式
- 用户的便签内容采用 Markdown 文件作为最终存储格式：每个笔记以 `notes/<note-id>/note.md` 保存其最新 checkpoint 文本（可直接用外部编辑器查看/备份）。
- 元数据（title、tags、last_modified、version）保存在 `state.json`。操作日志保存在 `journal.log`。这种文件化存储保证数据可移植、易于备份与导出，同时与 journal 恢复策略天然兼容。

### 应用启动与默认打开行为
- 在应用启动时，应根据 `index.json` 或全局 state 恢复上次会话信息：默认打开“最后被查看或编辑的笔记”（last-viewed note）。
  - 具体实现：在退出或后台时记录最后活动的 note-id 到 `state.json`（或 global_state.json）；启动时优先尝试加载该 note 并对其执行 journal replay 恢复最新内容。
- 如果记录的最后活动笔记不可用（文件被移动/删除或损坏），按以下顺序退回：
  1. 尝试打开上次最近修改的笔记（按照 index.json 的排序和 state.json 的 modified 时间）；
  2. 如果没有可用笔记，则打开空白新笔记。

### 新增窗口的语义
- 在应用内“新增窗口”操作语义为：创建并打开一个新的 note（对应文件夹 `notes/<new-id>/` 与 `note.md`）。
  - 新窗口应生成唯一 note-id、初始化 `state.json`（包含创建时间、owner、默认 tags）、并在 `index.json` 中注册此笔记。
  - 新窗口打开后，UI 中应将该窗口标记为“未保存”或“草稿”直到首次 checkpoint 完成。
- 另外：支持在新窗口中打开已有笔记的功能（通过文件树或双击打开），但默认行为为“新建笔记”。

### 全局预览窗口（Overview / Master Preview）
- 提供一个总的预览窗口，用于浏览/搜索/筛选用户的所有笔记：
  - 预览窗口基于 `index.json` 展示笔记列表与简短摘要（如 note 的首 N 行或 state 摘要）。
  - 支持快速搜索（基于 index.json 或可选的 SQLite 索引）、标签筛选、排序与分组操作。
  - 选择某项可在当前窗口打开该笔记或在新窗口打开（按用户选项）。
- 预览窗口应与其他窗口保持弱一致（event bus 推送更新）：当任一窗口对某笔记做 checkpoint 或结构性修改时，预览窗口应刷新该笔记的摘要/元数据显示。

### 额外注意事项
- 多窗口编辑同一笔记时，默认仍使用 journal + replay 策略合并变更；为降低冲突概率，推荐实现实时（进进程内）广播以减少需要 replay 的差异量。
- 在新建窗口和默认打开行为中，所有对 `index.json` 的修改都必须以原子方式写入（tmp -> rename）并做持久化，避免未完成的 index 修改导致丢失或不一致。


## 建议的下一步

- 将本文档作为 `docs/architecture.md` 保存；
- 基于 MVP-1 创建 plan 条目并分配任务与估时；
- 为 journal 恢复策略创建 ADR（记录为何采用 journal + checkpoint）。

- 将本文档作为 `docs/architecture.md` 保存；
- 基于 MVP-1 创建 plan 条目并分配任务与估时；
- 为 journal 恢复策略创建 ADR（记录为何采用 journal + checkpoint）。

