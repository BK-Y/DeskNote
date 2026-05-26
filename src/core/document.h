#ifndef DESKNOTE_DOCUMENT_H
#define DESKNOTE_DOCUMENT_H

#include <stddef.h>

/*
 * 最小文档模型。
 *
 * 当前阶段它只负责维护一段 UTF-16 文本缓冲区，
 * 不负责光标、选区、样式、渲染、平台消息。
 */
typedef struct {
    wchar_t* text;
    int length;
    int capacity;
} Document;

/*
 * 初始化空文档。
 *
 * 成功后文档满足：
 * - text != NULL
 * - length == 0
 * - text[0] == L'\0'
 */
int Document_Init(Document* doc);

/*
 * 释放文档持有的内存，并重置字段。
 */
void Document_Free(Document* doc);

/*
 * 清空文档内容，但保留已分配的缓冲区。
 */
int Document_Clear(Document* doc);

/*
 * 用一整段 UTF-16 文本替换当前文档内容。
 *
 * text 必须是以 `L'\0'` 结尾的字符串。
 */
int Document_SetText(Document* doc, const wchar_t* text);

/*
 * 获取文档当前文本内容。
 *
 * 返回值始终是以 `L'\0'` 结尾的 UTF-16 字符串。
 */
const wchar_t* Document_GetText(const Document* doc);

/*
 * 获取当前文档长度（不包含结尾的 `L'\0'`）。
 */
int Document_GetLength(const Document* doc);

/*
 * 在指定位置插入一个字符。
 *
 * index 合法范围：
 * - 0 <= index <= length
 */
int Document_InsertChar(Document* doc, int index, wchar_t ch);

/*
 * 删除指定位置的字符。
 *
 * index 合法范围：
 * - 0 <= index < length
 */
int Document_DeleteChar(Document* doc, int index);

#endif
