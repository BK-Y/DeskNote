# Phase 11 - 补基础选择（后续）

## 阶段摘要

把系统从“只有单点光标”推进到“支持最小文本范围选择”。

## 阶段目标

1. 支持键盘和鼠标下的基础选区
2. 支持拖拽选择
3. 支持双击选词
4. 保持选区、光标、视图三者状态一致

## 本阶段产出

1. 用户可以看到选区高亮
2. 鼠标拖拽可以形成连续选区
3. 双击一个词可以选中该词
4. 光标与选区切换规则明确

## 本阶段范围

1. 选区状态模型
2. 鼠标拖拽选择
3. 双击选词
4. 基础绘制高亮

## 本阶段不做

1. 列块选择
2. 多光标
3. Markdown 语义选择

## 分层归属

- `platform/`：收集按下、移动、释放、双击消息
- `app/`：串起选择主链
- `ui/`：命中区域与可视高亮组织
- `render/`：绘制选区高亮与测量
- `editor/`：持有选区状态与收束规则

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
鼠标 / 键盘选择消息
-> platform 收集
-> app 分发
-> ui 组织命中与可视区域
-> render 提供测量
-> editor 更新选区
-> app 请求重绘
```

## 完成标准

1. 可形成基础选区
2. 拖拽选择正常工作
3. 双击选词基本正确
4. 选区状态不打穿分层
