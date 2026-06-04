# Phase 13 - 补编辑命令面（后续）

## 阶段摘要

把系统从“基础编辑器”推进到“具备基础命令交互面”的编辑器。

## 阶段目标

1. 支持撤销重做
2. 支持右键菜单
3. 让基础编辑命令可被可视化触达

## 本阶段产出

1. 用户可执行撤销重做
2. 右键菜单具备基础编辑命令
3. 命令体系不再只依赖键盘路径

## 本阶段范围

1. 撤销栈 / 重做栈
2. 基础右键菜单
3. 菜单命令到编辑命令的映射

## 本阶段不做

1. 多光标撤销模型
2. Markdown 语义右键菜单
3. 插件化命令体系

## 分层归属

- `platform/`：收集右键与菜单命令
- `app/`：命令编排
- `ui/`：菜单语义与上下文组织
- `editor/`：撤销重做状态与命令执行
- `render/`：仅配合重绘

## 文件落点

### 本阶段预计修改文件

- `src/platform/win32/window.c`
- `src/platform/win32/window.h`
- `src/app/app.h`
- `src/app/app.c`
- `src/ui/editor_view.h`
- `src/ui/editor_view.c`
- `src/editor/editor.h`
- `src/editor/editor.c`

### 本阶段原则上不应修改

- `src/storage/note_store.c`
- `src/storage/state_store.c`

## 主链路

```text
菜单 / 快捷键命令
-> platform
-> app
-> editor 执行或回退命令
-> app 请求重绘
```

## 完成标准

1. 撤销重做可用
2. 右键菜单有基础编辑命令
3. 命令体系仍保持分层
