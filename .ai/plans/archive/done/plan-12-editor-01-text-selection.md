# Plan-12-editor-01 — 文本选择（拖拽 + 双击选词）

## ① 核心问题
编辑器支持单点光标，但无法用鼠标或键盘选中一段文本。用户无法复制内容、删除一段文字。

## ② 目标
- 鼠标拖拽可选中连续文本，松开后高亮保留
- 双击一个词可选中该词
- 选中后键盘输入/退格清除选区
- 选区高亮可视化

## ③ 方案设计
在 `Editor` 中新增选区状态 `{anchor_offset, active_offset, has_selection}`。

### 子步拆分
| 子步 | 内容 | 涉及层 | 预计行数 |
|------|------|--------|---------|
| ① | 选区状态模型：`EditorSelection` 定义 + 基础操作（设置/清除/读取） | editor 层 | ~30 |
| ② | 鼠标拖拽：`WM_LBUTTONDOWN` 设 anchor → `WM_MOUSEMOVE` 更新 active → `WM_LBUTTONUP` 保留 | app + platform 层 | ~50 |
| ③ | 选区高亮：`EditorView` 接收选区参数，在 `EditorView_Draw` 中用 `Render_FillRect` 绘制高亮 | ui 层 | ~40 |
| ④ | 双击选词：`WM_LBUTTONDBLCLK` → 词边界搜索 → 选中整个词 | app + editor 层 | ~30 |

鼠标按下时为选中开始，移动时更新 `active_offset`，释放后保留选区。

双击选词：收到 `WM_LBUTTONDBLCLK` 时，根据当前光标位置向前后搜索词边界（以空格/标点分隔），选中整个词。

## ④ 实现细节

### Editor 扩展
```c
typedef struct {
    int has_selection;
    int anchor_offset;   /* 选区起始位置（不随鼠标移动改变） */
    int active_offset;   /* 选区结束位置（随鼠标移动更新） */
} EditorSelection;
```

### 鼠标状态机
- `WM_LBUTTONDOWN` → 设置 `anchor_offset = active_offset = hit_position`，`has_selection = 1`
- `WM_MOUSEMOVE`（按下状态）→ 更新 `active_offset`，重绘高亮
- `WM_LBUTTONUP` → 保留选区，不改变光标
- `WM_LBUTTONDBLCLK` → 根据当前偏移搜索词边界，选区覆盖整个词

### 选区高亮
`EditorView_Draw` 接收选区参数，遍历文档行，对每行中被选中的区域用 `Render_FillRect` 绘制半透明蓝色背景。render 层不需要新增任何高亮专用函数。

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-1 | 鼠标拖拽选中 | 按住左键拖动过几行 | 选中区域高亮，松开后保留 |
| 2-2 | 双击选词 | 双击一个英文单词 | 整个单词被选中高亮 |
| 2-3 | 选中后输入覆盖 | 选中一段文字后按字母键 | 选区被清除，新字符插入 |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-4 | 空文档拖拽 | 在空文档中拖拽鼠标 | 无选区产生，不崩溃 |
| 2-5 | 选中到文档末尾 | 从中间拖到文档最末尾 | 选中范围正确，不越界 |

### 错误处理
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-6 | 快速双击后拖拽 | 双击后不松手直接拖拽 | 选区正常扩大，以双击词为起点 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-7 | 键盘输入不受影响 | 正常打字 | 输入正常，光标移动正常 |
| 2-8 | 标题栏按钮仍可点击 | 有选区时点击菜单 | 菜单弹出正常 |
| 2-9 | 编译通过 | `cmake --build build` | 零错误 |

### 集成
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-10 | 选中后切换模式 | 选中一段文字后切浮动置顶 | 不影响模式切换 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/editor/editor.h` | 新增 `EditorSelection` |
| `src/editor/editor.c` | 选区逻辑、词边界搜索 |
| `src/ui/editor_view.h` | `EditorView_Draw` 增加选区参数 |
| `src/ui/editor_view.c` | 遍历行 + `Render_FillRect` 绘制高亮 |
| `src/app/app.c` | 鼠标按下/移动/释放/双击分发 |

## ⑦ 推进步骤
1. 在 `editor.h` 定义 `EditorSelection`
2. 在 `editor_view.h` 修改 `EditorView_Draw` 签名增加选区参数
3. 在 `editor_view.c` 遍历文档行，用 `Render_FillRect` 画高亮
4. 在 `app.c` 实现鼠标状态机，绑定到 editor
5. 双击选词逻辑（词边界搜索在 editor.c）
6. 编译 + 验证

## ⑧ 分层核实
- editor 持有选区状态
- `EditorView_Draw` 根据选区坐标调 `Render_FillRect`（render 不知道 selection）
- `platform/*`、`storage/*` 零改动

## 验收标准

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
- [ ] [agent] `cmake --build build` 零错误

### 手工验证  [human 执行]
- [ ] [human] 测试 2-1~2-3（正常路径）全部通过
- [ ] [human] 测试 2-4~2-5（边界条件）全部通过
- [ ] [human] 测试 2-6（错误处理）全部通过
- [ ] [human] 测试 2-7~2-9（回归）全部通过
- [ ] [human] 测试 2-10（集成）全部通过

### GATE 1 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
