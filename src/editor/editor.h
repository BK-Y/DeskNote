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
    EDITOR_KEY_BACKSPACE,
    EDITOR_KEY_ENTER
} EditorKey;

/*
 * 最小编辑器状态。
 *
 * 当前只维护：
 * - 文本文档
 * - 光标插入点位置
 */
typedef struct {
    Document document;
    int cursor;
} Editor;

int Editor_Init(Editor* editor);
void Editor_Free(Editor* editor);

int Editor_SetText(Editor* editor, const wchar_t* text);
int Editor_HandleChar(Editor* editor, wchar_t ch);
int Editor_HandleKey(Editor* editor, EditorKey key);

const Document* Editor_GetDocument(const Editor* editor);
int Editor_GetCursor(const Editor* editor);

#endif
