#include "document.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define DOCUMENT_INITIAL_CAPACITY 64

static int Document_EnsureCapacity(Document* doc, int minimum_capacity)
{
    wchar_t* new_text;
    int new_capacity;

    if (doc == NULL || minimum_capacity <= 0)
        return 1;

    if (doc->capacity >= minimum_capacity)
        return 0;

    new_capacity = doc->capacity > 0 ? doc->capacity : DOCUMENT_INITIAL_CAPACITY;
    while (new_capacity < minimum_capacity)
        new_capacity *= 2;

    new_text = (wchar_t*)realloc(doc->text, (size_t)new_capacity * sizeof(wchar_t));
    if (new_text == NULL)
        return 1;

    doc->text = new_text;
    doc->capacity = new_capacity;
    return 0;
}

int Document_Init(Document* doc)
{
    if (doc == NULL)
        return 1;

    doc->text = (wchar_t*)malloc((size_t)DOCUMENT_INITIAL_CAPACITY * sizeof(wchar_t));
    if (doc->text == NULL)
    {
        doc->length = 0;
        doc->capacity = 0;
        return 1;
    }

    doc->length = 0;
    doc->capacity = DOCUMENT_INITIAL_CAPACITY;
    doc->text[0] = L'\0';
    return 0;
}

void Document_Free(Document* doc)
{
    if (doc == NULL)
        return;

    free(doc->text);
    doc->text = NULL;
    doc->length = 0;
    doc->capacity = 0;
}

int Document_Clear(Document* doc)
{
    if (doc == NULL || doc->text == NULL)
        return 1;

    doc->length = 0;
    doc->text[0] = L'\0';
    return 0;
}

int Document_SetText(Document* doc, const wchar_t* text)
{
    size_t length;

    if (doc == NULL || doc->text == NULL || text == NULL)
        return 1;

    length = wcslen(text);
    if (Document_EnsureCapacity(doc, (int)length + 1) != 0)
        return 1;

    memcpy(doc->text, text, (length + 1) * sizeof(wchar_t));
    doc->length = (int)length;
    return 0;
}

const wchar_t* Document_GetText(const Document* doc)
{
    if (doc == NULL || doc->text == NULL)
        return L"";

    return doc->text;
}

int Document_GetLength(const Document* doc)
{
    if (doc == NULL)
        return 0;

    return doc->length;
}

int Document_InsertChar(Document* doc, int index, wchar_t ch)
{
    size_t move_count;

    if (doc == NULL || doc->text == NULL)
        return 1;
    if (index < 0 || index > doc->length)
        return 1;
    if (Document_EnsureCapacity(doc, doc->length + 2) != 0)
        return 1;

    move_count = (size_t)(doc->length - index + 1);
    memmove(&doc->text[index + 1], &doc->text[index], move_count * sizeof(wchar_t));
    doc->text[index] = ch;
    doc->length += 1;
    return 0;
}

int Document_DeleteChar(Document* doc, int index)
{
    size_t move_count;

    if (doc == NULL || doc->text == NULL)
        return 1;
    if (index < 0 || index >= doc->length)
        return 1;

    move_count = (size_t)(doc->length - index);
    memmove(&doc->text[index], &doc->text[index + 1], move_count * sizeof(wchar_t));
    doc->length -= 1;
    return 0;
}
