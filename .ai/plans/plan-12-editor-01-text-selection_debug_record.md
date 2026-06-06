# text-selection_debug_record — 选区单击清除

> **关系**：本文件是 `done/plan-12-editor-01-text-selection.md`（点击定位+文本选择）的补充修复记录。
> 完整实现（选区模型 + 拖拽选词 + 高亮绘制）见 `done/` 下的主文件。

## 问题描述

有选区时（拖拽或双击选词后），单击其他位置不会清除选区。文本停留在高亮状态。

## 根因

`App_OnLeftButtonDown` 调用 `Editor_SetCursor` 移动了光标位置，但没有调用 `Editor_ClearSelection`。选区一直存在。

## 修复

在 `App_OnLeftButtonDown` 的 editor 定位分支中，`Editor_SetCursor` 之后、`App_ResultNeedsRefresh` 之前插入：

```c
if (Editor_HasSelection(&g_app.editor))
    Editor_ClearSelection(&g_app.editor);
```

## 涉及文件

- `src/app/app.c`：`App_OnLeftButtonDown` editor 分支加清除逻辑

## 验收标准

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 如涉及启动应用，确保旧进程已关闭

### 自动化检查  [agent 执行]
- [ ] [agent] `cmake --build build` 零错误

### 手工验证  [human 执行]
- [ ] [human] 拖拽选中一段文本 → 单击其他位置 → 选区被清除，光标移动到单击位置
- [ ] [human] 双击选中一个词 → 单击其他位置 → 选区被清除

### GATE 1 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
