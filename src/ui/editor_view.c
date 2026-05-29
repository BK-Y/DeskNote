#include "editor_view.h"

typedef struct {
    RenderRect editor_rect;
    RenderRect text_rect;
} EditorViewLayout;

static RenderRect g_client_area_rect;
static int g_has_client_area_rect = 0;

static EditorViewLayout EditorView_CalculateLayout(int window_width, int window_height)
{
    const int margin = 0;           /* Shell-2b_1: 铺满客户区 */
    const int top_margin = 0;       /* Shell-2b_1: 铺满客户区 */
    const int text_horizontal_padding = 16;
    const int text_vertical_padding = 0;  /* Shell-2b_1: 去掉上下 padding */
    RenderRect client_area_rect;
    EditorViewLayout layout;

    if (g_has_client_area_rect)
    {
        client_area_rect = g_client_area_rect;
    }
    else
    {
        client_area_rect.x = 0;
        client_area_rect.y = 0;
        client_area_rect.width = window_width;
        client_area_rect.height = window_height;
    }

    layout.editor_rect.x = client_area_rect.x + margin;
    layout.editor_rect.y = client_area_rect.y + top_margin;
    layout.editor_rect.width = client_area_rect.width - (margin * 2);
    layout.editor_rect.height = client_area_rect.height - (top_margin + margin);

    if (layout.editor_rect.width < 0)
        layout.editor_rect.width = 0;
    if (layout.editor_rect.height < 0)
        layout.editor_rect.height = 0;

    layout.text_rect.x = layout.editor_rect.x + text_horizontal_padding;
    layout.text_rect.y = layout.editor_rect.y + text_vertical_padding;
    layout.text_rect.width = layout.editor_rect.width - (text_horizontal_padding * 2);
    layout.text_rect.height = layout.editor_rect.height - (text_vertical_padding * 2);

    if (layout.text_rect.width < 0)
        layout.text_rect.width = 0;
    if (layout.text_rect.height < 0)
        layout.text_rect.height = 0;

    return layout;
}

int EditorView_GetTextRect(int window_width, int window_height, RenderRect* out_rect)
{
    EditorViewLayout layout;

    if (out_rect == NULL)
        return 1;

    layout = EditorView_CalculateLayout(window_width, window_height);
    *out_rect = layout.text_rect;
    return 0;
}

int EditorView_GetContentHeight(RenderContext* ctx,
                                const Document* document,
                                int window_width,
                                int window_height,
                                int* out_height)
{
    EditorViewLayout layout;
    const wchar_t* text;
    int length;

    if (ctx == NULL || document == NULL || out_height == NULL)
        return 1;

    layout = EditorView_CalculateLayout(window_width, window_height);
    text = Document_GetText(document);
    length = Document_GetLength(document);
    if (length <= 0)
    {
        *out_height = 0;
        return 0;
    }

    return Render_MeasureTextHeight(ctx, text, length, layout.text_rect, out_height);
}

int EditorView_GetCaretRect(RenderContext* ctx,
                            const Document* document,
                            int cursor,
                            int vertical_scroll,
                            int window_width,
                            int window_height,
                            RenderRect* out_rect)
{
    const int caret_width = 2;
    const int caret_min_height = 12;
    EditorVisualPosition visual_position;
    int caret_height;

    if (ctx == NULL || document == NULL || out_rect == NULL)
        return 1;
    if (EditorView_GetCursorVisualPosition(ctx,
                                           document,
                                           cursor,
                                           vertical_scroll,
                                           window_width,
                                           window_height,
                                           &visual_position) != 0)
    {
        return 1;
    }

    out_rect->x = visual_position.x;
    caret_height = visual_position.line_height > caret_min_height ?
        (visual_position.line_height - 4) : visual_position.line_height;
    if (caret_height < caret_min_height)
        caret_height = caret_min_height;

    out_rect->y = visual_position.y +
                  ((visual_position.line_height - caret_height + 1) / 2);
    out_rect->width = caret_width;
    out_rect->height = caret_height;
    return 0;
}

int EditorView_HitTestCursor(RenderContext* ctx,
                             const Document* document,
                             int window_width,
                             int window_height,
                             int vertical_scroll,
                             int x,
                             int y,
                             int* out_cursor)
{
    EditorViewLayout layout;
    const wchar_t* text;
    int length;
    int hit_x;
    int hit_y;
    RenderRect content_rect;

    if (ctx == NULL || document == NULL || out_cursor == NULL)
        return 1;

    layout = EditorView_CalculateLayout(window_width, window_height);
    if (x < layout.editor_rect.x || y < layout.editor_rect.y ||
        x >= layout.editor_rect.x + layout.editor_rect.width ||
        y >= layout.editor_rect.y + layout.editor_rect.height)
    {
        return 1;
    }

    hit_x = x;
    hit_y = y;
    if (layout.text_rect.width > 0)
    {
        if (hit_x < layout.text_rect.x)
            hit_x = layout.text_rect.x;
        if (hit_x > layout.text_rect.x + layout.text_rect.width - 1)
            hit_x = layout.text_rect.x + layout.text_rect.width - 1;
    }
    if (layout.text_rect.height > 0)
    {
        if (hit_y < layout.text_rect.y)
            hit_y = layout.text_rect.y;
        if (hit_y > layout.text_rect.y + layout.text_rect.height - 1)
            hit_y = layout.text_rect.y + layout.text_rect.height - 1;
    }

    content_rect = layout.text_rect;
    content_rect.y -= vertical_scroll;
    text = Document_GetText(document);
    length = Document_GetLength(document);

    return Render_HitTestPoint(ctx, text, length, content_rect, hit_x, hit_y, out_cursor);
}

int EditorView_GetCursorVisualPosition(RenderContext* ctx,
                                       const Document* document,
                                       int cursor,
                                       int vertical_scroll,
                                       int window_width,
                                       int window_height,
                                       EditorVisualPosition* out_position)
{
    EditorViewLayout layout;
    const wchar_t* text;
    int length;
    RenderTextPosition text_position;
    RenderRect content_rect;

    if (ctx == NULL || document == NULL || out_position == NULL)
        return 1;

    layout = EditorView_CalculateLayout(window_width, window_height);
    content_rect = layout.text_rect;
    content_rect.y -= vertical_scroll;
    text = Document_GetText(document);
    length = Document_GetLength(document);

    if (cursor < 0)
        cursor = 0;
    if (cursor > length)
        cursor = length;

    if (Render_HitTestTextPosition(ctx, text, length, content_rect, cursor, &text_position) != 0)
        return 1;

    out_position->x = text_position.x;
    out_position->y = text_position.y;
    out_position->line_height = text_position.height > 0 ? text_position.height : 18;
    return 0;
}

void EditorView_Draw(RenderContext* ctx,
                     const Document* document,
                     int cursor,
                     int vertical_scroll,
                     int window_width,
                     int window_height)
{
    EditorViewLayout layout;
    const wchar_t* text;
    int length;
    RenderRect draw_rect;

    (void)cursor;
    layout = EditorView_CalculateLayout(window_width, window_height);

    Render_FillRect(ctx, layout.editor_rect, (RenderColor){ 255, 251, 214, 255 });

    text = Document_GetText(document);
    length = Document_GetLength(document);

    if (length <= 0)
    {
        Render_DrawText(ctx,
                        L"DeskNote editor",
                        layout.text_rect,
                        (RenderColor){ 110, 110, 110, 255 });
    }
    else
    {
        draw_rect = layout.text_rect;
        draw_rect.y -= vertical_scroll;
        Render_PushClip(ctx, layout.text_rect);
        Render_DrawText(ctx,
                        text,
                        draw_rect,
                        (RenderColor){ 40, 40, 40, 255 });
        Render_PopClip(ctx);
    }
}

void EditorView_OnClientAreaChanged(int x, int y, int width, int height)
{
    g_client_area_rect.x = x;
    g_client_area_rect.y = y;
    g_client_area_rect.width = width;
    g_client_area_rect.height = height;
    g_has_client_area_rect = 1;
}