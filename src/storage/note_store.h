#ifndef DESKNOTE_NOTE_STORE_H
#define DESKNOTE_NOTE_STORE_H

#include <windows.h>

int NoteStore_GetDefaultNotePath(wchar_t* buffer, int buffer_count);
int NoteStore_LoadText(const wchar_t* path, wchar_t** out_text);
int NoteStore_SaveText(const wchar_t* path, const wchar_t* text);
void NoteStore_FreeText(wchar_t* text);

#endif
