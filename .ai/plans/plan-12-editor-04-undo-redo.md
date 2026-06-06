# Plan-12-editor-04 — 撤销 / 重做

## ① 核心问题
编辑器不可撤销。误操作无法回退，用户每步操作都是永久性的。

## ② 目标
- Ctrl+Z 撤销，Ctrl+Shift+Z / Ctrl+Y 重做
- 编辑历史栈（环形、固定深度如 100）
- 插入/删除/粘贴自动入栈
- 撤销后若编辑则丢弃后续重做记录

## ③ 方案设计
在 `Editor` 中维护编辑操作历史栈。

### 子步拆分
| 子步 | 内容 | 涉及层 | 预计行数 |
|------|------|--------|---------|
| ① | 操作栈结构：`EditOperation` + `EditHistory` 定义 + `PushUndo` / `Undo` / `Redo` 实现 | editor 层 | ~80 |
| ② | 快捷键绑定：`App_OnKeyDown` 中映射 Ctrl+Z / Ctrl+Y | app 层 | ~10 |

为减少内存占用，用操作记录替代完整文档快照。

为减少内存占用，用"操作记录"（`char_inserted`, `char_deleted` 等）替代完整快照——对简单文本编辑器来说，操作记录足够且内存开销低。

## ④ 实现细节

```c
typedef enum {
    EDIT_OP_INSERT,
    EDIT_OP_DELETE
} EditOpType;

typedef struct {
    EditOpType type;
    int offset;
    int length;            /* 操作涉及的字符数 */
    wchar_t text[64];      /* 被插入或删除的文本（前64字符） */
    int text_len;
} EditOperation;
```

> 对于 >64 字符的大段操作，记录前后文档快照的 diff，或直接限制单次操作范围（当前编辑器单次输入不会超过 64 字符）。

typedef struct {
    EditOperation ops[100];
    int undo_top;         /* 撤销栈顶索引 */
    int redo_top;         /* 重做栈顶索引 */
    int count;            /* 当前栈中操作数 */
} EditHistory;
```

快捷键绑定在 `App_OnKeyDown` 中，检查 `VK_Z` + `Ctrl`。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 4-1 | 撤销插入 | 输入 "abc" → Ctrl+Z | "abc" 消失 |
| 4-2 | 重做 | Ctrl+Z 后 Ctrl+Y | "abc" 恢复 |
| 4-3 | 撤销删除 | 退格删除 → Ctrl+Z | 被删字符恢复 |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 4-4 | 空历史撤销 | 打开新文档按 Ctrl+Z | 无操作，不报错 |
| 4-5 | 栈满 | 连续做 101 次操作 | 最早的记录被丢弃 |
| 4-6 | 撤销后编辑清重做 | Ctrl+Z → 输入新字符 → Ctrl+Y | Ctrl+Y 不可用（重做栈已清） |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 4-7 | 普通编辑不受影响 | 正常打字 | 编辑流畅，无延迟 |
| 4-8 | 编译通过 | `cmake --build build` | 零错误 |

### 集成
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 4-9 | 撤销选区替换 | 选中一段文字 → 输入 → Ctrl+Z | 选区文字恢复 |
| 4-10 | 多光标撤销 | 多光标操作后 Ctrl+Z | 回到操作前状态 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/editor/editor.h` | 新增 `EditOperation` / `EditHistory` |
| `src/editor/editor.c` | 入栈/撤销/重做逻辑 |
| `src/app/app.c` | `App_OnKeyDown` 快捷键映射 |

## ⑦ 推进步骤
1. 在 `editor.h` 定义操作类型和栈结构
2. 在 `editor.c` 实现 `Editor_PushUndo` / `Editor_Undo` / `Editor_Redo`
3. 在 `editor.c` 的插入/删除函数中调用入栈
4. 在 `app.c` 绑定 Ctrl+Z / Ctrl+Y
5. 编译 + 验证

## ⑧ 分层核实
- editor 持有历史栈和撤销语义
- app 层转发快捷键
- `platform/*`、`render/*`、`storage/*` 零改动

## 验收标准

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
- [ ] [agent] `cmake --build build` 零错误零警告
- [ ] [agent] `gcc -fsyntax-only src/editor/editor.c` 语法通过
- [ ] [agent] `gcc -fsyntax-only src/app/app.c` 语法通过

### 手工验证  [human 执行]
- [ ] [human] 测试 4-1~4-3（正常路径）全部通过
- [ ] [human] 测试 4-4~4-6（边界条件）全部通过
- [ ] [human] 测试 4-7~4-8（回归）全部通过
- [ ] [human] 测试 4-9~4-10（集成）全部通过

### GATE 4 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
