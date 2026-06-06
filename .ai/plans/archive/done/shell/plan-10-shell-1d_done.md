# Shell-1d — 标题栏组件与边框视觉基线（后续）

## 阶段摘要

把系统从"frameless 基线已经落地、编辑器正文区已正确缩进"推进到"标题栏组件已存在、边框视觉已自绘、三区（标题栏/边框/正文）布局关系已确立"的状态。

## 阶段目标

1. 新增 `ui/titlebar.h` + `ui/titlebar.c`，对外输出标题栏布局计算和绘制能力
2. 窗口顶部出现自绘标题栏区域，高度 = `titlebar_height`，含标题文本和按钮视觉
3. 窗口边缘出现自绘边框（`frame_visual_thickness` 宽），不再依赖系统 frame
4. 标题栏按钮面（关闭/最小化按钮）以视觉状态出现，为 Shell-2a 的交互命中预留布局基线
5. 将 `src/ui/titlebar.c` 接入 CMakeLists.txt 构建，产物通过编译

## 为什么要做这个

1. `Shell-1c` 和 `Shell-1c-repair-1` 完成后，窗口虽是 frameless，但顶区只是空白的非客户区，没有标题栏视觉、没有按钮、没有边框——看起来不像便签
2. 如果直接跳去 `Shell-2a` 做拖拽/缩放/按钮命中，会因为标题栏和边框还没有稳定的视觉布局而反复回改
3. 这个子阶段必须单独存在，把"空的 frameless 窗体"升级为"有标题栏和边框视觉基线的窗体"，后续 Shell-2a/2b 的计算（按钮命中、拖拽热区）才能建立在已知的几何参数上

## 阶段产出

1. 启动后窗口顶部出现自绘标题栏背景，标题文本"DeskNote"可见
2. 窗口边缘出现 1px 自绘边框（颜色区别于标题栏和正文背景）
3. 标题栏右上角出现关闭按钮和最小化按钮的视觉区域（按钮无交互，仅为视觉占位）
4. 调整窗口大小时，标题栏和边框自动跟随重绘，正文区不受干扰
5. 标题栏组件的布局参数（标题栏矩形、按钮矩形）可通过 `TitlebarLayout` 结构体由外部读取，供后续 Shell-2a 消费

## 本阶段范围

1. `src/ui/titlebar.h` 和 `src/ui/titlebar.c` 的新增与实现
2. 标题栏布局计算（标题栏矩形、标题文本矩形、按钮矩形）
3. 标题栏背景、按钮视觉、标题文本的绘制
4. 窗口四边 1px 边框的自绘
5. `app.c` 绘制链中接入 `Titlebar_Draw`
6. `CMakeLists.txt` 中新增 `src/ui/titlebar.c`

## 本阶段不做

1. 不修改 frameless / client-area 已有主链（`window.c` / `nonclient.c` 不动）
2. 不修改 `editor_view.c/h`（正文区矩形已由 `ComputeContentRect` 正确计算，titlebar 层不改变其输入）
3. 不新增 render 层绘制原语（`Render_FillRect` + `Render_DrawText` 已满足所有绘制需求）
4. 不处理按钮点击 / 拖拽 / 缩放 / 双击标题栏等交互命中（Shell-2a 承接）
5. 不接入托盘 / topmost / AppBar 行为
6. 不修改 `app.h` 结构体和不新增 app 层接口

## 分层归属

### `ui/titlebar`（新增）

- 负责标题栏布局计算
- 负责标题栏背景、按钮、标题文本的绘制
- 负责窗口四边边框的绘制
- 调用 render 层的 `Render_FillRect`、`Render_DrawText`、`Render_PushClip/PopClip`
- 不处理任何输入事件

### `app/`

- 在 `App_OnPaint` 的 `Render_Clear` 之后、`EditorView_Draw` 之前插入 `Titlebar_Draw` 调用
- 通过 `App_GetShellState` 向 titlebar 提供 `titlebar_height`、`frame_visual_thickness`、`titlebar_command_groups` 参数
- 不做标题栏语义判断

### `render/`

- 保持现有接口集不变
- 被 titlebar 调用的原语与被 editor_view 调用的原语相同
- 不理解标题栏/边框的业务语义

### `platform/`

- 本阶段不参与

### `editor/`

- 本阶段不参与

### `storage/`

- 本阶段不参与

### `core/`

- 本阶段不参与

## 文件落点

### 本阶段新增文件

- `src/ui/titlebar.h` — 标题栏组件头文件，声明布局输出接口和绘制接口
- `src/ui/titlebar.c` — 标题栏组件实现

### 本阶段预计修改文件

- `src/app/app.c` — 在 `App_OnPaint` 中插入 titlebar 绘制调用
- `CMakeLists.txt` — 在 `SOURCES` 中添加 `src/ui/titlebar.c`

### 本阶段原则上不应修改

- `src/app/app.h`（无需新增接口，`AppShellState`/`App_GetShellState` 已有）
- `src/ui/editor_view.h` / `editor_view.c`（正文区输入不变）
- `src/render/render.h` / `render.c`（现有原语已满足，不新增）
- `src/platform/win32/window.c`（frameless 主链不动）
- `src/platform/win32/nonclient.c`（non-client 指标不动）
- `src/storage/state_store.c` / `state_store.h`
- `src/storage/note_store.c` / `note_store.h`
- `src/storage/storage_write.c`
- `src/editor/editor.c` / `editor.h`
- `src/core/document.c` / `document.h`

## 技术路线

### 新增入口文件

**`src/ui/titlebar.h`** — 新增标题栏组件接口：

```c
#ifndef DESKNOTE_TITLEBAR_H
#define DESKNOTE_TITLEBAR_H

#include "../render/render.h"

/*
 * 标题栏布局计算结果。
 * Shell-1d 只提供视觉布局，不处理命中。
 * 按钮矩形供 Shell-2a 消费。
 */
typedef struct {
    RenderRect titlebar_rect;          // 标题栏背景区域（整行宽度，titlebar_height 高度）
    RenderRect title_text_rect;        // 标题文本显示区域
    RenderRect close_button_rect;      // 关闭按钮视觉区域
    RenderRect minimize_button_rect;   // 最小化按钮视觉区域
    int has_close_button;
    int has_minimize_button;
} TitlebarLayout;

/*
 * 根据窗口参数计算标题栏布局。
 * window_width / window_height : 窗口客户区尺寸
 * titlebar_height              : 标题栏高度（像素）
 * frame_visual_thickness       : 边框视觉宽度（像素）
 * titlebar_command_groups      : 位掩码，来自 AppShellState.titlebar_command_groups
 */
TitlebarLayout Titlebar_CalculateLayout(int window_width,
                                        int window_height,
                                        int titlebar_height,
                                        int frame_visual_thickness,
                                        unsigned int titlebar_command_groups);

/*
 * 绘制标题栏（背景、标题文本、按钮）和窗口边框。
 * - 顶部边框：从 (0,0) 到 (width, frame_visual_thickness)
 * - 标题栏背景：从 (0, frame_visual_thickness) 到 (width, frame_visual_thickness + titlebar_height)
 * - 左右下边框分别绘制在窗口边缘
 * 调用者需确保 ctx 已处于 BeginFrame/EndFrame 之间。
 */
void Titlebar_Draw(RenderContext* ctx, const TitlebarLayout* layout);

#endif
```

**`src/ui/titlebar.c`** — 实现标题栏组件，只依赖 `render.h`：
- `Titlebar_CalculateLayout`：根据窗口尺寸和 `titlebar_command_groups` 位掩码计算所有矩形
  - `has_close_button` / `has_minimize_button` 根据 `APP_TITLEBAR_COMMAND_GROUP_WINDOW_CONTROLS` 位决定
  - 按钮排列：右上角水平排列，按钮尺寸固定（如 46×titlebar_height），间距固定
  - `title_text_rect` = titlebar 区域左侧预留部分
- `Titlebar_Draw`：按 layout 结构体绘制
  - 顶部边框：`Render_FillRect`，颜色 `(180, 180, 180, 255)`
  - 标题栏背景：`Render_FillRect`，颜色 `(255, 251, 214, 255)`（与编辑区一致）
  - 标题文本：`Render_DrawText`，内容 "DeskNote"，颜色 `(80, 80, 80, 255)`
  - 按钮：`Render_FillRect` 绘制按钮矩形，颜色区分 hover 状态（Shell-1d 统一用默认色）
  - 左右下边框：`Render_FillRect`，颜色 `(180, 180, 180, 255)`
  - 按钮区使用 `Render_PushClip` / `Render_PopClip` 确保不溢出标题栏区域

### 状态承接文件

- `src/app/app.c`（现有文件）
  - 不新增字段，不新增接口
  - `App_OnPaint` 中在 `Render_Clear` 之后、`EditorView_Draw` 之前插入 titlebar 绘制链：
    ```c
    void App_OnPaint(void)
    {
        if (g_app.render == NULL)
            return;
        if (Render_BeginFrame(g_app.render) != 0)
            return;

        Render_Clear(g_app.render, (RenderColor){ 240, 240, 240, 255 });

        // [新增] 绘制标题栏和边框
        {
            TitlebarLayout titlebar_layout;
            unsigned int width = 0, height = 0;
            if (App_GetClientSize(&width, &height) == 0)
            {
                titlebar_layout = Titlebar_CalculateLayout(
                    (int)width, (int)height,
                    g_app.shell.titlebar_height,
                    g_app.shell.frame_visual_thickness,
                    g_app.shell.titlebar_command_groups);
                Titlebar_Draw(g_app.render, &titlebar_layout);
            }
        }

        if (g_app.editor_ready && g_app.has_focus)
            App_UpdateInputCaret();
        EditorView_Draw(g_app.render,
                        Editor_GetDocument(&g_app.editor),
                        g_app.editor.cursor,
                        g_app.vertical_scroll,
                        (int)width, (int)height);

        Render_EndFrame(g_app.render);
    }
    ```
  - 注意：上述代码的 `width` / `height` 变量需从 `App_GetClientSize` 统一获取，并在两个绘制调用间共享

### 调度 / 测量 / 适配文件

- `src/app/app.c`（见上）
- `CMakeLists.txt`
  - 在 `SOURCES` 中添加一行：`src/ui/titlebar.c`

### 旧路径调整

- **无旧路径替换**。当前 `App_OnPaint` 中无标题栏绘制逻辑，不存在覆盖或替换关系
- `App_OnPaint` 中原有的 `Render_Clear` / `EditorView_Draw` 主链保持不动，titlebar 绘制作为新增层插入中间
- `Shell-1b_done` 保留的最小顶区拖拽占位路径（通过 `Platform_Nonclient_HandleNCHitTest` 返回 `HTCAPTION`）继续保持，本阶段不改变；正式替换为 titlebar 组件命中交给 `Shell-2a`

## 主链路

```text
WM_PAINT
-> app 收到绘制事件
-> app 调用 Render_BeginFrame
-> app 调用 Render_Clear (窗口背景)

-> app 从 AppShellState 读取 titlebar_height / frame_visual_thickness / titlebar_command_groups
-> [新增] app 调用 Titlebar_CalculateLayout → 获得 TitlebarLayout
-> [新增] app 调用 Titlebar_Draw
   -> titlebar 调用 Render_FillRect（绘制边框、标题栏背景、按钮）
   -> titlebar 调用 Render_DrawText（绘制标题文本）
   -> titlebar 调用 Render_PushClip / PopClip（按钮区域裁剪）

-> app 调用 EditorView_Draw（绘制正文区）

-> app 调用 Render_EndFrame
```

## 完成标准

1. **标题栏组件文件存在** — `src/ui/titlebar.h` 和 `src/ui/titlebar.c` 已创建，并通过编译
2. **标题栏区域可见** — 启动后窗口顶部出现 `titlebar_height` 高度的自绘标题栏，背景色为 `(255, 251, 214, 255)`，标题文本 "DeskNote" 在左侧可见
3. **边框区域可见** — 窗口四边出现 `frame_visual_thickness` 宽的自绘边框线，颜色 `(180, 180, 180, 255)`，与编辑器背景可区分
4. **按钮视觉存在** — 标题栏右上角出现关闭按钮和最小化按钮的视觉矩形，颜色与标题栏背景不同，按钮无需响应点击
5. **窗口 resize 后持久** — 拖动窗口边缘改变大小时，标题栏区域和边框跟随重绘，不出现撕裂或空白
6. **正文区不受影响** — `EditorView_Draw` 的内容区矩形未因 titlebar 的加入而偏移或错位（通过 `App_RequestClientRebuild` 保障）
7. **构建通过** — `cmake --build build` 无错误无警告

## 计划的最后一步

1. `git add` 当前阶段修改过的所有文件（`src/ui/titlebar.h` / `src/ui/titlebar.c` / `src/app/app.c` / `CMakeLists.txt` / `.ai/phases/phase-shell-1d.md`）
2. `git commit` 使用规范提交说明记录这次阶段实施
3. `git push` 把阶段更新提交到远端仓库
4. 如果当前阶段确认已完成，把对应计划文件重命名为追加 `_done` 后缀的文件名
