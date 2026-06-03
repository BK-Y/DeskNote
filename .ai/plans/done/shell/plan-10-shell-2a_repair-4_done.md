# Shell-2a_repair-4 — render 层新增 Render_DrawTextCentered 居中绘制函数（后续）

## 阶段摘要

在 render.h/render.c 中新增 Render_DrawTextCentered 函数，支持文本在矩形区域内水平垂直居中绘制。

## 阶段目标

1. render.h 中新增 Render_DrawTextCentered 声明
2. render.c 中实现 Render_DrawTextCentered（复用 Render_PrepareText + Render_CreateTextLayout + 设置 CENTER 对齐 + DrawTextLayout）
3. render.h 头部补充 API 总览注释（按分类列出所有函数 + 边界约束）

## 为什么要做这个

1. 当前 Render_DrawText 始终使用左对齐+顶对齐，按钮 label、标题文本等需要居中的场景无法通过它实现
2. Shell-2a_repair-5（button 居中）和 Shell-2a_repair-6（titlebar 配色）都依赖这个函数，必须先落地
3. 在 render 层实现居中复用现有内部函数（Render_CreateTextLayout），不需要暴露 DWrite 内部细节给 ui 层

## 阶段产出

1. render.h 头部多一段 API 总览注释，所有函数按分类列出
2. render.h 中多一个 Render_DrawTextCentered 声明
3. render.c 中多一个 Render_DrawTextCentered 实现
4. 编译通过后，上层组件可调用该函数实现居中文字

## 本阶段范围

只改 render.h/render.c。不改 button.c、titlebar.c、app/、platform/ 任何文件。

## 本阶段不做

1. 不修改 Render_DrawText 的签名或行为
2. 不修改 Render_Init 中的全局 text_format 对齐设置
3. 不改动任何调用方（button.c、titlebar.c、editor_view.c）
4. 不新增 render 层除居中外的其他能力

## 分层归属

### render/

- 新增 Render_DrawTextCentered 函数
- 复用 Render_PrepareText、Render_CreateTextLayout 等内部函数
- 在创建的临时 IDWriteTextLayout 上设置 CENTER 对齐，不影响全局 text_format
- 在 render.h 头部新增 API 总览注释

### ui/ / app/ / platform/ / editor/ / storage/ / core/

- 本阶段不参与

## 文件落点

### 本阶段预计修改文件

- src/render/render.h — 新增 Render_DrawTextCentered 声明 + API 总览注释
- src/render/render.c — 实现 Render_DrawTextCentered

### 本阶段原则上不应修改

- src/ui/*、src/app/*、src/platform/*、src/storage/*、src/editor/*、src/core/*

## 技术路线

### render.h

文件头部新增 API 总览注释：

```c
/*
 * ── render 层 API 总览 ─────────────────────────────────────────────
 * 生命周期: Create, Destroy, Init, Shutdown, Resize
 * 帧控制:   BeginFrame, EndFrame
 * 清屏:     Clear
 * 矩形:     FillRect
 * 裁剪:     PushClip, PopClip
 * 文字绘制: DrawText (左对齐+顶对齐), DrawTextCentered (水平垂直居中)
 * 文本测量: HitTestTextPosition, HitTestPoint, MeasureTextHeight
 * ─────────────────────────────────────────────────────────────────
 * 边界: render 层不知道 titlebar / button / cursor / selection 等业务概念
 */
```

然后在 render.h 正文末尾或合适位置新增声明：

```c
void Render_DrawTextCentered(RenderContext* ctx,
                             const wchar_t* text,
                             RenderRect rect,
                             RenderColor color);
```

### render.c

实现函数，复用内部流程：

```c
void Render_DrawTextCentered(RenderContext* ctx,
                             const wchar_t* text,
                             RenderRect rect,
                             RenderColor color)
{
    ID2D1RenderTarget* base_target;
    IDWriteTextLayout* text_layout;
    RenderPreparedText prepared;
    D2D1_COLOR_F d2d_color;
    D2D1_POINT_2F origin;
    UINT32 length;

    if (ctx == NULL || ctx->render_target == NULL || ctx->brush == NULL ||
        ctx->text_format == NULL || text == NULL)
        return;

    base_target = (ID2D1RenderTarget*)ctx->render_target;
    d2d_color = Render_ToD2DColor(color);
    length = (UINT32)wcslen(text);
    if (Render_PrepareText(text, length, 0, &prepared) != 0)
        return;
    text_layout = Render_CreateTextLayout(ctx, prepared.text, prepared.length, rect);
    if (text_layout == NULL)
    {
        Render_FreePreparedText(&prepared);
        return;
    }

    text_layout->lpVtbl->SetTextAlignment(text_layout, DWRITE_TEXT_ALIGNMENT_CENTER);
    text_layout->lpVtbl->SetParagraphAlignment(text_layout, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    origin.x = (FLOAT)rect.x;
    origin.y = (FLOAT)rect.y;

    ctx->brush->lpVtbl->SetColor(ctx->brush, &d2d_color);
    base_target->lpVtbl->DrawTextLayout(base_target, origin, text_layout, ctx->brush);
    Render_ReleaseUnknown((void**)&text_layout);
    Render_FreePreparedText(&prepared);
}
```

注意：此函数只修改临时 IDWriteTextLayout 的对齐属性，不影响全局 text_format。

## 主链路

```text
上层组件（button / titlebar 等）调用 Render_DrawTextCentered
-> render 层创建临时 IDWriteTextLayout
-> 设置水平居中 + 垂直居中
-> 以 rect.x/rect.y 为起点绘制
-> 释放临时 text_layout
```

## 完成标准

1. 编译通过（新增函数 + 0 warning，现有调用方不受影响）
2. render.h 头部可见 API 总览注释
3. render.h 中存在 Render_DrawTextCentered 声明

## 计划的最后一步

1. git add + commit -m "Shell-2a_repair-4: add Render_DrawTextCentered + API overview"
2. git push
3. 完成后重命名为 _done 后缀
