# Phase 12 - 补高级浏览（后续）

## 阶段摘要

把系统从“最小可浏览”推进到“滚动体验更稳定、更完整”。

## 阶段目标

1. 补复杂平滑滚动
2. 补横向滚动
3. 收紧长文本下的视口模型

## 本阶段产出

1. 滚动更连续
2. 超宽内容可横向浏览
3. 长文本下视口行为更稳定

## 本阶段范围

1. 平滑滚动
2. 横向滚动
3. 滚动与视图刷新节奏收紧

## 本阶段不做

1. 选择系统
2. Markdown 富文本命中
3. 多光标

## 分层归属

- `platform/`：收集滚轮与相关系统消息
- `app/`：持有滚动调度
- `ui/`：组织视口语义
- `render/`：基于视口绘制
- `editor/`：只配合光标与视口联动

## 文件落点

### 本阶段预计修改文件

- `src/platform/win32/window.c`
- `src/app/app.h`
- `src/app/app.c`
- `src/ui/editor_view.h`
- `src/ui/editor_view.c`
- `src/render/render.h`
- `src/render/render.c`
- `src/editor/editor.h`
- `src/editor/editor.c`

### 本阶段原则上不应修改

- `src/storage/note_store.c`
- `src/storage/state_store.c`

## 主链路

```text
滚动消息
-> platform
-> app
-> ui 组织视口
-> render 绘制
-> app 同步光标可见性
```

## 完成标准

1. 平滑滚动可用
2. 横向滚动可用
3. 长文本滚动下光标与视图仍一致
