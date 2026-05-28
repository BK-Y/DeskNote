#ifndef DESKNOTE_STORAGE_WRITE_H
#define DESKNOTE_STORAGE_WRITE_H

#include <windows.h>

int StorageWriteUtf16FileAtomic(const wchar_t* path, const wchar_t* text);

#endif
