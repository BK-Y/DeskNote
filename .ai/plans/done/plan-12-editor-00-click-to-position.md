# phase-11-editor-click-to-position — 点击定位光标

## ① 核心问题

当前键盘（方向键、IME）可以移动光标，但鼠标点击文本区域不会把光标移到点击位置。用户必须用键盘定位编辑点，不符合直觉。

## ② 目标

鼠标左键点击编辑器区域时，光标移动到点击位置；点击后光标闪烁、输入从该位置开始。

## ③ 方案设计

平台层已转发 `App_OnLeftButtonDown(x, y)`，`app.c` 中已做 titlebar hit test 并调用 `EditorView_HitTestTextPosition`。当前缺失的是：hit test 后的光标更新 + 重绘。

只需要在 `App_OnLeftButtonDown` 的编辑器区域命中分支中补上：

1. 调用 `Render_HitTestPoint(...)` 获取点击位置的文本偏移
2. 调用 editor 函数更新光标位置
3. 调用 `App_EnsureCaretVisible()` 确保光标可见
4. 触发重绘

## ④ 现有可复用接口

| 接口 | 位置 | 用途 |
|------|------|------|
| `Render_HitTestPoint(...)` | `render.h` | 通过像素坐标 → 文本偏移 |
| `Editor_GetCursor(...)` | `editor.h` | 获取当前光标偏移 |
| `Editor_SetCursor(...)` | `editor.c` | 设置光标偏移（需确认存在） |
| `App_EnsureCaretVisible()` | `app.c` | 滚动到光标可见 |
| `App_RequestRefresh()` | `app.c` | 触发重绘 |

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 前置条件 | 操作 | 预期 |
|------|------|---------|------|------|
| 1-1 | 点击已有文本 | 文档中有多行文本 | 鼠标点击某行中间 | 光标移到点击位置，闪烁 |
| 1-2 | 点击行尾 | 同一文档 | 点击行尾右侧空白 | 光标放在该行最后一个字符后 |
| 1-3 | 点击空行 | 文档中有空行 | 点击空行任意位置 | 光标移到该空行 |

### 边界条件
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 1-4 | 点击文档末尾空白 | 点击最后一行下方 | 光标放在文档末尾 |
| 1-5 | 点击文档开头之前 | 点击第一行上方 | 光标放在文档开头 |

### 错误处理
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 1-6 | 点击窗口非编辑区 | 点击标题栏或边框 | 光标不移动，不崩溃 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 1-7 | 点击后键盘输入 | 点击定位 + 打字 | 文字出现在点击位置 |
| 1-8 | 点击后方向键移动 | 点击定位 + 左右键 | 光标正常移动 |
| 1-9 | 编译通过 | `cmake --build build` | 零错误 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/app/app.c` | `App_OnLeftButtonDown` 中编辑器区域分支补全 hit test + 设光标 + 确保可见 + 重绘 |

不应修改：`src/storage/*`、`src/platform/*`、`src/render/*`、`src/editor/*`。

## ⑦ 推进步骤

1. 在 `App_OnLeftButtonDown` 中找到 `TITLEBAR_HIT_NONE` 的 else 分支（当前是空实现或 TODO）
2. 调用 `Render_HitTestPoint(...)` 获取文本偏移
3. 调用编辑器设置光标 + 清除选区（如果有）
4. 调 `App_EnsureCaretVisible()` + `App_RequestRefresh()`
5. 编译验证

## ⑧ 分层核实

- app 层调 `Render_HitTestPoint`（render 公开接口）+ editor 设置光标（editor 公开接口）+ 平台 `SetCaretPos`（平台公开接口）
- 无反向调用，不修改 render/editor 内部状态
