# phase-12-markdown-hit-test — Markdown 富文本命中

## ① 核心问题
编辑器将所有文本视为纯文本。点击链接、勾选框、标题等语义区域时，无特殊交互行为。

## ② 目标
- 区分普通文本区域和链接/勾选框/标题等 Markdown 语义区域
- 点击不同语义区域时能识别其类型（用于后续交互）
- 纯文本命中与语义命中由同一 hit test 路径处理

## ③ 方案设计
在 `core/document` 层维护 Markdown 语法树（`MdAst`）。`Render_HitTestPoint` 在返回字符偏移的同时，额外检查该偏移是否落在一个语义节点内；若是，填充 `MdSemanticInfo`（含类型：链接、勾选框、标题等）。

## ④ 实现细节
```c
typedef enum {
    MD_HIT_NONE = 0,
    MD_HIT_LINK,
    MD_HIT_CHECKBOX,
    MD_HIT_HEADING,
    MD_HIT_CODE_SPAN
} MdHitType;

typedef struct {
    MdHitType type;
    int start_offset;
    int end_offset;
    const wchar_t* link_url;  /* 仅 MD_HIT_LINK 时有效 */
} MdSemanticInfo;
```

`Render_HitTestPoint` 返回的 `int* out_text_position` 是文本偏移；新增 `MdSemanticInfo* out_semantic` 输出语义信息（可为 NULL，向后兼容）。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 12-1 | 点击链接文本 | Markdown 文档中有 `[link](url)`，点击 "link" | `out_semantic->type == MD_HIT_LINK` |
| 12-2 | 点击普通文本 | 点击非语义区域的文本 | `out_semantic->type == MD_HIT_NONE` |
| 12-3 | 点击勾选框 | 文档中有 `- [ ] task`，点击 `[ ]` | `out_semantic->type == MD_HIT_CHECKBOX` |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 12-4 | 纯文本文档 | 无 Markdown 语法的文档 | 所有点击 `MD_HIT_NONE` |
| 12-5 | 嵌套语义 | 标题中包含链接 `# [title](url)` | 按最近节点类型返回 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 12-6 | 纯文本编辑不变 | 正常打字 | 编辑不受影响 |
| 12-7 | 编译通过 | `cmake --build build` | 零错误 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/core/document.h` | 新增 `MdSemanticInfo` / `MdHitType` |
| `src/core/document.c` | 语法树存储与查询 |
| `src/render/render.h` | `Render_HitTestPoint` 增加 `out_semantic` 参数 |
| `src/render/render.c` | 语义命中逻辑 |

## ⑦ 推进步骤
1. 接入 `lib/md4c` 解析器生成 AST
2. 在 `document` 层提供 `Document_GetSemanticAt(offset)` 查询接口
3. 在 `render` 层 hit test 中调用并填充 `out_semantic`
4. 编译 + 验证

## ⑧ 分层核实
- `core/` 提供语义数据，不依赖任何上层
- `render/` 在 hit test 中查询语义信息
- `platform/*`、`storage/*` 零改动
