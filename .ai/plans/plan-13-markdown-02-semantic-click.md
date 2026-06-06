# Plan-13-markdown-02 — 语义点击行为

## ① 核心问题
Markdown 语义区域能量中，但点击后无对应行为。点击链接不跳转、点击勾选框不切换状态。

## ② 目标
- 点击链接：用默认浏览器打开 URL
- 点击勾选框 `[ ]` / `[x]`：切换选中状态并更新文档
- 点击标题行：可快速定位（后续）
- 语义行为和纯文本编辑行为互不干扰

## ③ 方案设计
在 `App_OnLeftButtonDown` 中，hit test 返回语义信息后，根据 `MdHitType` 分派行为：
- `MD_HIT_LINK`：检查 Ctrl+点击 / 直接点击 → `ShellExecuteW(hwnd, "open", url, ...)`
- `MD_HIT_CHECKBOX`：切换文档中对应位置的 `[ ]↔[x]`，触发重绘
- `MD_HIT_NONE` / 其他：走原有纯文本点击定位逻辑

## ④ 实现细节
- 链接打开限制：仅 Ctrl+点击打开（防止误触）；或直接点击打开（取决于默认行为定义）
- 勾选框切换：`Editor_ReplaceTextInRange(offset, 4, new_checkbox_text)`——替换 `[ ]` 为 `[x]` 或反过来的 4 字符操作
- 链接/勾选框点击后不移动光标（光标位置不变）

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 13-8 | 点击链接 | Markdown 有 `[link](https://example.com)`，点击 "link" | 浏览器打开 `https://example.com` |
| 13-9 | 点击勾选框 | `- [ ] task` → 点击 `[ ]` | 文档变为 `- [x] task` |
| 13-10 | 再次点击勾选框 | `- [x] task` → 点击 `[x]` | 文档变为 `- [ ] task` |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 13-11 | 点击无 URL 链接 | `[broken]()` 或 URL 为空 | 不打开，不崩溃 |
| 13-12 | 编辑区内链接 | 光标在链接文本中，按住 Ctrl 点击 | 打开链接，不移动光标 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 13-13 | 纯文本编辑不受影响 | 正常编辑 | 不受影响 |
| 13-14 | 编译通过 | `cmake --build build` | 零错误 |

### 集成
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 13-15 | Alt 键不影响点击 | 按住 Alt 点击链接 | 正常处理 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/app/app.c` | `App_OnLeftButtonDown` 中根据 `MdHitType` 分派处理 |
| `src/editor/editor.c` | 新增 `Editor_ReplaceTextInRange` 用于勾选框切换 |

## ⑦ 推进步骤
1. 在 `App_OnLeftButtonDown` 中接收语义信息
2. 实现 `MdHitType` 的分派处理
3. 实现勾选框切换的编辑操作
4. 编译 + 验证

## ⑧ 分层核实
- app 层根据语义类型分派行为
- editor 层执行勾选框状态切换等编辑动作
- `platform/*`、`render/*`、`storage/*` 零改动

## 验收标准

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
- [ ] [agent] `cmake --build build` 零错误零警告
- [ ] [agent] `gcc -fsyntax-only src/app/app.c` 语法通过
- [ ] [agent] `gcc -fsyntax-only src/editor/editor.c` 语法通过
- [ ] [agent] 确认 `Editor_ReplaceTextInRange` 接口已声明

### 手工验证  [human 执行]
- [ ] [human] 测试 13-8~13-10（正常路径）全部通过
- [ ] [human] 测试 13-11~13-12（边界条件）全部通过
- [ ] [human] 测试 13-13~13-14（回归）全部通过
- [ ] [human] 测试 13-15（集成）全部通过

### GATE 2 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
