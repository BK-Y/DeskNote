#include "editor.h"

int Editor_Init(Editor* editor)
{
    if (editor == NULL)
        return 1;

    editor->cursor = 0;
    return Document_Init(&editor->document);
}

void Editor_Free(Editor* editor)
{
    if (editor == NULL)
        return;

    Document_Free(&editor->document);
    editor->cursor = 0;
}

int Editor_SetText(Editor* editor, const wchar_t* text)
{
    if (editor == NULL)
        return 1;

    if (Document_SetText(&editor->document, text) != 0)
        return 1;

    editor->cursor = Document_GetLength(&editor->document);
    return 0;
}

int Editor_HandleChar(Editor* editor, wchar_t ch)
{
    if (editor == NULL)
        return 1;

    if (Document_InsertChar(&editor->document, editor->cursor, ch) != 0)
        return 1;

    editor->cursor += 1;
    return 0;
}

int Editor_HandleKey(Editor* editor, EditorKey key)
{
    if (editor == NULL)
        return 1;

    switch (key)
    {
    case EDITOR_KEY_LEFT:
        if (editor->cursor <= 0)
            return 1;
        editor->cursor -= 1;
        return 0;

    case EDITOR_KEY_RIGHT:
        if (editor->cursor >= Document_GetLength(&editor->document))
            return 1;
        editor->cursor += 1;
        return 0;

    case EDITOR_KEY_BACKSPACE:
        if (editor->cursor <= 0)
            return 1;
        if (Document_DeleteChar(&editor->document, editor->cursor - 1) != 0)
            return 1;
        editor->cursor -= 1;
        return 0;

    case EDITOR_KEY_ENTER:
        return Editor_HandleChar(editor, L'\n');
    }

    return 1;
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
