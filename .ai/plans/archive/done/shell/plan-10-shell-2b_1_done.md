# Shell-2b_1 — 默认窗口 360x500 + 编辑区背景铺满（后续）

## 阶段摘要

将默认窗口大小从 800x600 调整为 240x388，并将编辑区黄色背景铺满整个内容区。

## 阶段目标

1. CreateWindowExW 默认尺寸改为 240x388
2. EditorView_CalculateLayout 中 editor_rect 的 margin/top_margin 改为 0
3. text_rect 保留左右 padding 16px（防止文字贴边），去掉上下 padding

## 为什么要做这个

1. 默认 800x600 是工具窗口尺寸，240x388（黄金比例近似 240/388 ≈ 0.618）更符合便签的视觉预期
2. 当前 editor_rect 有 24px margin，导致黄色背景四周有一圈灰色背景暴露——便签纸应该是从边到边的，不应有外露的系统背景色

## 阶段产出

1. 启动后窗口为 240x388，不再是 800x600
2. 黄色背景从窗口左上角铺到右下角，无灰色边缘
3. 文字距左右窗口边缘 16px，不会贴边

## 本阶段范围

只改 window.c 的默认尺寸和 editor_view.c 的布局参数。不改 app/、render/、titlebar/ 任何文件。

## 本阶段不做

1. 不改窗口缩放功能（Shell-2b_2/3 做）
2. 不改标题栏布局
3. 不改边框绘制
4. 不改 editor_view 的文本测量和命中逻辑

## 分层归属

### ui/editor_view

- EditorView_CalculateLayout 中 margin=0, top_margin=0, text_vertical_padding=0

### platform/win32/window

- CreateWindowExW 的默认宽高改为 360x500

## 文件落点

### 本阶段预计修改文件

- src/platform/win32/window.c — 默认宽高 800x600 → 240x388
- src/ui/editor_view.c — margin=0, top_margin=0, text_vertical_padding=0

### 本阶段原则上不应修改

- src/app/*、src/render/*、src/editor/*、src/storage/*、src/core/*、src/ui/titlebar.*

## 技术路线

### window.c

```c
HWND hwnd = CreateWindowExW(
    0,
    L"DeskNoteWindow",
    L"DeskNote",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    240, 388,       /* 默认便签尺寸 240x388（≈黄金比例）*/
    NULL, NULL, instance, NULL);
```

### editor_view.c

```c
static EditorViewLayout EditorView_CalculateLayout(int window_width, int window_height)
{
    const int margin = 0;
    const int top_margin = 0;
    const int text_horizontal_padding = 16;
    const int text_vertical_padding = 0;
    ...
}
```

注意：editor_rect 的 x/y 已由 g_client_area_rect 缩进过（标题栏高度 + 边框宽度），margin=0 不会覆盖到标题栏区域。

## 主链路

```text
程序启动
-> CreateWindowExW 创建 360x500 窗口
-> App_OnPaint → Titlebar_Draw（标题栏+边框）
-> App_OnPaint → EditorView_Draw
-> EditorView_Draw 用 editor_rect 画黄色背景（margin=0，铺满）
-> EditorView_Draw 用 text_rect 画正文（保留左右 padding）
```

## 完成标准

1. 启动后窗口大小为 360x500
2. 黄色编辑区背景从窗口左上角铺到右下角，无灰色边缘
3. 文字不与左右窗口边缘贴边（保留左右 16px padding）
4. 标题栏和边框绘制正常

## 计划的最后一步

1. git add + commit + push
2. 完成后重命名为 _done 后缀
