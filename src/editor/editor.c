#include "editor.h"

static void Editor_ClearPreferredColumn(Editor* editor)
{
    if (editor == NULL)
        return;

    editor->preferred_x = 0;
    editor->has_preferred_x = 0;
}

int Editor_Init(Editor* editor)
{
    if (editor == NULL)
        return 1;

    editor->cursor = 0;
    editor->preferred_x = 0;
    editor->has_preferred_x = 0;
    return Document_Init(&editor->document);
}

void Editor_Free(Editor* editor)
{
    if (editor == NULL)
        return;

    Document_Free(&editor->document);
    editor->cursor = 0;
    editor->preferred_x = 0;
    editor->has_preferred_x = 0;
}

static EditorResult Editor_MoveCursorHorizontal(Editor* editor, int delta)
{
    int target_cursor;
    int length;

    if (editor == NULL)
        return EDITOR_RESULT_ERROR;

    length = Document_GetLength(&editor->document);
    target_cursor = editor->cursor + delta;
    if (target_cursor < 0)
        target_cursor = 0;
    if (target_cursor > length)
        target_cursor = length;
    if (target_cursor == editor->cursor)
        return EDITOR_RESULT_UNCHANGED;

    editor->cursor = target_cursor;
    Editor_ClearPreferredColumn(editor);
    return EDITOR_RESULT_CURSOR_MOVED;
}

EditorResult Editor_SetText(Editor* editor, const wchar_t* text)
{
    if (editor == NULL)
        return EDITOR_RESULT_ERROR;

    if (Document_SetText(&editor->document, text) != 0)
        return EDITOR_RESULT_ERROR;

    editor->cursor = Document_GetLength(&editor->document);
    Editor_ClearPreferredColumn(editor);
    return EDITOR_RESULT_TEXT_CHANGED;
}

EditorResult Editor_SetCursor(Editor* editor, int cursor)
{
    int length;

    if (editor == NULL)
        return EDITOR_RESULT_ERROR;

    length = Document_GetLength(&editor->document);
    if (cursor < 0)
        cursor = 0;
    if (cursor > length)
        cursor = length;
    if (cursor == editor->cursor)
        return EDITOR_RESULT_UNCHANGED;

    editor->cursor = cursor;
    Editor_ClearPreferredColumn(editor);
    return EDITOR_RESULT_CURSOR_MOVED;
}

EditorResult Editor_HandleChar(Editor* editor, wchar_t ch)
{
    if (editor == NULL)
        return EDITOR_RESULT_ERROR;

    if (Document_InsertChar(&editor->document, editor->cursor, ch) != 0)
        return EDITOR_RESULT_ERROR;

    editor->cursor += 1;
    Editor_ClearPreferredColumn(editor);
    return EDITOR_RESULT_TEXT_CHANGED;
}

EditorResult Editor_HandleKey(Editor* editor, EditorKey key)
{
    if (editor == NULL)
        return EDITOR_RESULT_ERROR;

    switch (key)
    {
    case EDITOR_KEY_LEFT:
        return Editor_MoveCursorHorizontal(editor, -1);

    case EDITOR_KEY_RIGHT:
        return Editor_MoveCursorHorizontal(editor, 1);

    case EDITOR_KEY_UP:
    case EDITOR_KEY_DOWN:
        return EDITOR_RESULT_ERROR;

    case EDITOR_KEY_BACKSPACE:
        if (editor->cursor <= 0)
            return EDITOR_RESULT_UNCHANGED;
        if (Document_DeleteChar(&editor->document, editor->cursor - 1) != 0)
            return EDITOR_RESULT_ERROR;
        editor->cursor -= 1;
        Editor_ClearPreferredColumn(editor);
        return EDITOR_RESULT_TEXT_CHANGED;

    case EDITOR_KEY_ENTER:
        return Editor_HandleChar(editor, L'\n');
    }

    return EDITOR_RESULT_ERROR;
}

EditorResult Editor_MoveCursorVertical(Editor* editor,
                                       const EditorVerticalNavigator* navigator,
                                       int direction)
{
    EditorVisualPosition current_position;
    int target_cursor;
    int target_x;
    int target_y;

    if (editor == NULL || navigator == NULL)
        return EDITOR_RESULT_ERROR;
    if (navigator->get_visual_position == NULL || navigator->hit_test_point == NULL)
        return EDITOR_RESULT_ERROR;
    if (direction != -1 && direction != 1)
        return EDITOR_RESULT_ERROR;
    if (navigator->get_visual_position(navigator->context, editor->cursor, &current_position) != 0)
        return EDITOR_RESULT_ERROR;

    if (!editor->has_preferred_x)
    {
        editor->preferred_x = current_position.x;
        editor->has_preferred_x = 1;
    }

    target_x = editor->preferred_x;
    target_y = current_position.y + (current_position.line_height / 2) +
               (direction * current_position.line_height);
    if (navigator->hit_test_point(navigator->context, target_x, target_y, &target_cursor) != 0)
        return EDITOR_RESULT_ERROR;
    if (target_cursor == editor->cursor)
        return EDITOR_RESULT_UNCHANGED;

    editor->cursor = target_cursor;
    return EDITOR_RESULT_CURSOR_MOVED;
}

const Document* Editor_GetDocument(const Editor* editor)
{
    if (editor == NULL)
        return NULL;

    return &editor->document;
}

int Editor_GetCursor(const Editor* editor)
{
    if (editor == NULL)
        return 0;

    return editor->cursor;
}
