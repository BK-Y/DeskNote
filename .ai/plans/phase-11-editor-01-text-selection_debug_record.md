# text-selection_debug_record — 选区单击清除

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
