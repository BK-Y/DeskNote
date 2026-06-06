# Plan-12-editor-03 — 滚动体验优化

## ① 核心问题
当前滚轮滚动是离散的（每格固定步长 48px），无平滑过渡。超宽内容无法横向浏览。

## ② 目标
- 滚轮滚动更平滑（小步长 or 动画过渡）
- 支持横向滚动（Shift+滚轮）
- 长文本下闪烁光标始终跟随视口

## ③ 方案设计
修改 `App_OnMouseWheel` 将步长从固定 48px 改为可配置。

### 子步拆分
| 子步 | 内容 | 涉及层 | 预计行数 |
|------|------|--------|---------|
| ① | 平滑步长：`scroll_step` 从 48 改为 24 | app 层 | ~5 |
| ② | 横向滚动：检测 Shift+滚轮 → `horizontal_scroll` 状态 → EditorView 偏移计算 | app + ui 层 | ~50 |

横向滚动复用 `vertical_scroll` 的逻辑模型。

## ④ 实现细节
- `App_OnMouseWheel` 中 `scroll_step` 可从常数改为静态变量，通过设置界面后续调整
- 横向滚动复用 `vertical_scroll` 的逻辑模型（`AppState` 中新增 `horizontal_scroll`）
- `EditorView_Draw` 和 `EditorView_GetTextRect` 根据 `horizontal_scroll` 做横向偏移
- 视口跟随光标：`App_EnsureCaretVisible` 扩展为同时检查水平和垂直可见性

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 3-1 | 滚轮滚动 | 滚动鼠标滚轮 | 内容平滑上下移动 |
| 3-2 | Shift+滚轮横向 | 按住 Shift 滚动 | 内容左右移动 |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 3-3 | 文档很短时滚轮 | 不足一屏的文档中滚动 | 无越界行为，不崩溃 |
| 3-4 | 超宽文本 | 一行超过窗口宽度的文本 | 横向滚动可见全部内容 |

### 错误处理
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 3-5 | 快速连续滚轮 | 快速连续滚动 | 无跳跃、不丢帧 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 3-6 | 光标跟随 | 键盘移动光标到屏幕外 | 自动滚动到光标可见 |
| 3-7 | 编译通过 | `cmake --build build` | 零错误 |

### 集成
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 3-8 | 滚动后点击定位 | 滚动到底部后点击上半部分 | 光标定位正确 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/app/app.c` | `App_OnMouseWheel` 平滑步长 + 横向分支 |
| `src/ui/editor_view.h` | 视口新增 `horizontal_scroll` 参数 |
| `src/ui/editor_view.c` | 横向偏移计算 |
| `src/render/render.h` | 视口裁剪增强 |

## ⑦ 推进步骤
1. 在 `AppState` 添加 `horizontal_scroll`
2. 修改 `App_OnMouseWheel` 支持 Shift+滚轮横向滚动
3. 修改 `EditorView_Draw` 根据 `horizontal_scroll` 偏移
4. 修改 `App_EnsureCaretVisible` 检查横向可见性
5. 编译 + 手动验证

## ⑧ 分层核实
- scroll 状态保存在 `AppState`（app 层）
- `EditorView` 负责坐标偏移（ui 层）
- `Render` 根据偏移绘制（render 层）
- `platform/*` 仅转发滚轮消息，零改动

## 验收标准

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
- [ ] [agent] `cmake --build build` 零错误零警告
- [ ] [agent] `gcc -fsyntax-only src/app/app.c` 语法通过
- [ ] [agent] `gcc -fsyntax-only src/ui/editor_view.c` 语法通过

### 手工验证  [human 执行]
- [ ] [human] 测试 3-1~3-2（正常路径）全部通过
- [ ] [human] 测试 3-3~3-4（边界条件）全部通过
- [ ] [human] 测试 3-5（错误处理）全部通过
- [ ] [human] 测试 3-6~3-7（回归）全部通过
- [ ] [human] 测试 3-8（集成）全部通过

### GATE 3 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
