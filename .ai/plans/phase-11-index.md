# Phase 11-12 索引

## 状态图例

| 标记 | 含义 |
|------|------|
| ✅ 就绪 | 所有 API 已就绪，可直接实施 |
| ⚠️ 可分步 | 可做，建议按单列的子步顺序推进 |
| ⏳ 阻塞 | 依赖前面的计划完成后才能做 |
| 🗄️ 已归档 | 已完成（文件在 `done/`） |

---

## 模块 1 — 编辑器核心（Phase-11-editor）

### 已归档

| 文件 | 名称 | 说明 |
|------|------|------|
| `phase-11-editor-00-click-to-position.md` 🗄️ | 点击定位光标 | 代码已实现（`done/`） |
| `phase-11-editor-01-text-selection.md` 🗄️ | 文本选择 | 代码已实现（`done/`） |

### 待实施

| 文件 | 名称 | 状态 | 子步 |
|------|------|------|------|
| `phase-11-editor-02-clipboard.md` | 剪贴板操作与快捷键 | ✅ 就绪 | ①Ctrl+C/V → ②Ctrl+X+Delete → ③Delete向后删 |
| `phase-11-editor-03-scroll.md` | 滚动体验优化 | ⚠️ 拆 2 步 | ①平滑步长 → ②横向滚动 |
| `phase-11-editor-04-undo-redo.md` | 撤销 / 重做 | ⚠️ 拆 2 步 | ①操作栈 → ②快捷键 |
| `phase-11-editor-05-context-menu.md` | 右键菜单 | ⏳ 阻塞 | 等待剪贴板完成 |
| `phase-11-editor-06-multi-cursor.md` | 多光标 | ⏳ 阻塞 | 等待选择 + 撤销完成 |

---

## 模块 2 — Markdown 语义（Phase-12-markdown）

| 文件 | 名称 | 状态 | 说明 |
|------|------|------|------|
| `phase-12-markdown-01-hit-test.md` | 富文本命中 | ⏳ 阻塞 | 需先重新接入 `lib/md4c` 解析器 |
| `phase-12-markdown-02-semantic-click.md` | 语义点击行为 | ⏳ 阻塞 | 等待命中完成 |

---

## 模块 3 — Repair：模块化拆分（Phase-13-split）

| 文件 | 名称 | 状态 | 说明 |
|------|------|------|------|
| `phase-13-split-shell.md` | 拆出 shell 模块 | ⏳ 阻塞 | 将模式切换逻辑从 app.c 迁入 src/shell/ |
| `phase-13-split-editor-nav.md` | 迁入编辑器导航 | ⏳ 阻塞 | 将 App_Navigator* 迁入 src/editor/ |

---

## 推荐实施顺序

```
第1步: 02-clipboard (已就绪)           ← 当前切入点
       03-scroll 子步① (48→24)          ← 可并行
       04-undo-redo 子步① (操作栈)      ← 可并行
  ↓
第2步: 03-scroll 子步② · 04-undo 子步②
  ↓
第3步: 05-context-menu
  ↓
第4步: 06-multi-cursor
  ↓
第5步: Markdown 模块
  ↓
第6步: Repair：模块化拆分
```


