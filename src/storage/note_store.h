#ifndef DESKNOTE_NOTE_STORE_H
#define DESKNOTE_NOTE_STORE_H

#include <windows.h>

typedef enum {
    NOTE_RECOVERY_LOAD_OK = 0,
    NOTE_RECOVERY_LOAD_NOT_FOUND = 1,
    NOTE_RECOVERY_LOAD_INVALID = 2,
    NOTE_RECOVERY_LOAD_ERROR = 3
} NoteRecoveryLoadResult;

typedef struct {
    unsigned int version;
    wchar_t* text;
} NoteRecoverySnapshot;

int NoteStore_GetDefaultNotePath(wchar_t* buffer, int buffer_count);
int NoteStore_LoadText(const wchar_t* path, wchar_t** out_text);
int NoteStore_SaveText(const wchar_t* path, const wchar_t* text);
NoteRecoveryLoadResult NoteStore_LoadRecoverySnapshot(const wchar_t* note_path,
                                                      NoteRecoverySnapshot* out_snapshot);
int NoteStore_SaveRecoverySnapshot(const wchar_t* note_path,
                                   unsigned int version,
                                   const wchar_t* text);
int NoteStore_DeleteRecoverySnapshot(const wchar_t* note_path);
void NoteStore_FreeText(wchar_t* text);
void NoteStore_FreeRecoverySnapshot(NoteRecoverySnapshot* snapshot);

#endif
