#ifndef DESKNOTE_EDITOR_H
#define DESKNOTE_EDITOR_H

#include "../core/document.h"

/*
 * 编辑器当前支持的最小控制键集合。
 *
 * 这些枚举由 app 层从平台按键翻译而来，
 * editor 层本身不直接依赖 Win32 的 VK_* 常量。
 */
typedef enum {
    EDITOR_KEY_LEFT = 1,
    EDITOR_KEY_RIGHT,
    EDITOR_KEY_UP,
    EDITOR_KEY_DOWN,
    EDITOR_KEY_BACKSPACE,
    EDITOR_KEY_ENTER
} EditorKey;

typedef enum {
    EDITOR_RESULT_ERROR = -1,
    EDITOR_RESULT_UNCHANGED = 0,
    EDITOR_RESULT_CURSOR_MOVED = 1,
    EDITOR_RESULT_TEXT_CHANGED = 2
} EditorResult;

typedef struct {
    int x;
    int y;
    int line_height;
} EditorVisualPosition;

typedef struct {
    void* context;
    int (*get_visual_position)(void* context, int cursor, EditorVisualPosition* out_position);
    int (*hit_test_point)(void* context, int x, int y, int* out_cursor);
} EditorVerticalNavigator;

/*
 * 最小编辑器状态。
 *
 * 当前维护：
 * - 文本文档
 * - 光标插入点位置
 * - 纵向移动列锚点
 */
typedef struct {
    Document document;
    int cursor;
    int preferred_x;
    int has_preferred_x;
} Editor;

int Editor_Init(Editor* editor);
void Editor_Free(Editor* editor);

EditorResult Editor_SetText(Editor* editor, const wchar_t* text);
EditorResult Editor_SetCursor(Editor* editor, int cursor);
EditorResult Editor_HandleChar(Editor* editor, wchar_t ch);
EditorResult Editor_HandleKey(Editor* editor, EditorKey key);
EditorResult Editor_MoveCursorVertical(Editor* editor,
                                       const EditorVerticalNavigator* navigator,
                                       int direction);

const Document* Editor_GetDocument(const Editor* editor);
int Editor_GetCursor(const Editor* editor);

#endif
