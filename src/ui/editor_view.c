#include "editor_view.h"

typedef struct {
    RenderRect editor_rect;
    RenderRect text_rect;
} EditorViewLayout;

static EditorViewLayout EditorView_CalculateLayout(int window_width, int window_height)
{
    const int margin = 24;
    const int top_margin = 24;
    const int text_horizontal_padding = 16;
    const int text_vertical_padding = 12;
    EditorViewLayout layout;

    layout.editor_rect.x = margin;
    layout.editor_rect.y = top_margin;
    layout.editor_rect.width = window_width - (margin * 2);
    layout.editor_rect.height = window_height - (top_margin + margin);

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

int EditorView_GetCaretRect(RenderContext* ctx,
                            const Document* document,
                            int cursor,
                            int window_width,
                            int window_height,
                            RenderRect* out_rect)
{
    const int caret_width = 2;
    const int caret_min_height = 12;
    const wchar_t* text;
    RenderTextPosition text_position;
    EditorViewLayout layout;
    int line_height;
    int caret_height;
    int length;

    if (ctx == NULL || document == NULL || out_rect == NULL)
        return 1;

    layout = EditorView_CalculateLayout(window_width, window_height);
    text = Document_GetText(document);
    length = Document_GetLength(document);

    if (cursor < 0)
        cursor = 0;
    if (cursor > length)
        cursor = length;

    if (Render_HitTestTextPosition(ctx, text, length, layout.text_rect, cursor, &text_position) != 0)
        return 1;

    out_rect->x = text_position.x;
    line_height = text_position.height > 0 ? text_position.height : 18;
    caret_height = line_height > caret_min_height ? (line_height - 4) : line_height;
    if (caret_height < caret_min_height)
        caret_height = caret_min_height;

    out_rect->y = text_position.y + ((line_height - caret_height + 1) / 2);
    out_rect->width = caret_width;
    out_rect->height = caret_height;
    return 0;
}

void EditorView_Draw(RenderContext* ctx,
                     const Document* document,
                     int cursor,
                     int window_width,
                     int window_height)
{
    EditorViewLayout layout;
    const wchar_t* text;
    int length;

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
        Render_DrawText(ctx,
                        text,
                        layout.text_rect,
                        (RenderColor){ 40, 40, 40, 255 });
    }
}