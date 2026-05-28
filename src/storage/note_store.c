#include "note_store.h"

#include "state_store.h"
#include "storage_write.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define NOTE_STORE_FILE_NAME L"note.md"
#define NOTE_STORE_RECOVERY_SUFFIX L".recovery"

typedef enum {
    NOTE_STORE_READ_OK = 0,
    NOTE_STORE_READ_NOT_FOUND = 1,
    NOTE_STORE_READ_INVALID = 2,
    NOTE_STORE_READ_ERROR = 3
} NoteStoreReadResult;

static int NoteStore_CopyString(wchar_t* buffer, int buffer_count, const wchar_t* text)
{
    size_t length;

    if (buffer == NULL || buffer_count <= 0 || text == NULL)
        return 1;

    length = wcslen(text);
    if ((int)length >= buffer_count)
        return 1;

    memcpy(buffer, text, (length + 1) * sizeof(wchar_t));
    return 0;
}

static int NoteStore_CombinePath(wchar_t* buffer,
                                 int buffer_count,
                                 const wchar_t* left,
                                 const wchar_t* right)
{
    size_t left_length;
    size_t right_length;

    if (buffer == NULL || buffer_count <= 0 || left == NULL || right == NULL)
        return 1;

    left_length = wcslen(left);
    right_length = wcslen(right);
    if ((int)(left_length + 1 + right_length) >= buffer_count)
        return 1;

    memcpy(buffer, left, left_length * sizeof(wchar_t));
    buffer[left_length] = L'\\';
    memcpy(buffer + left_length + 1, right, (right_length + 1) * sizeof(wchar_t));
    return 0;
}

static int NoteStore_EnsureDirectoryExists(const wchar_t* path)
{
    DWORD attributes;

    if (path == NULL || path[0] == L'\0')
        return 1;

    attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            return 1;
        return 0;
    }

    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
}

static int NoteStore_EnsureParentDirectoryExists(const wchar_t* path)
{
    wchar_t directory_path[MAX_PATH];
    wchar_t* separator;

    if (path == NULL || path[0] == L'\0')
        return 1;
    if (NoteStore_CopyString(directory_path, MAX_PATH, path) != 0)
        return 1;

    separator = wcsrchr(directory_path, L'\\');
    if (separator == NULL)
        return 1;

    *separator = L'\0';
    return NoteStore_EnsureDirectoryExists(directory_path);
}

static int NoteStore_AllocateEmptyText(wchar_t** out_text)
{
    wchar_t* text;

    if (out_text == NULL)
        return 1;

    text = (wchar_t*)malloc(sizeof(wchar_t));
    if (text == NULL)
        return 1;

    text[0] = L'\0';
    *out_text = text;
    return 0;
}

static int NoteStore_BuildRecoveryPath(const wchar_t* note_path,
                                       wchar_t* recovery_path,
                                       int recovery_path_count)
{
    size_t note_path_length;
    size_t suffix_length;

    if (note_path == NULL || recovery_path == NULL || recovery_path_count <= 0)
        return 1;

    note_path_length = wcslen(note_path);
    suffix_length = wcslen(NOTE_STORE_RECOVERY_SUFFIX);
    if ((int)(note_path_length + suffix_length) >= recovery_path_count)
        return 1;

    if (NoteStore_CopyString(recovery_path, recovery_path_count, note_path) != 0)
        return 1;

    memcpy(recovery_path + note_path_length,
           NOTE_STORE_RECOVERY_SUFFIX,
           (suffix_length + 1) * sizeof(wchar_t));
    return 0;
}

static NoteStoreReadResult NoteStore_LoadUtf16File(const wchar_t* path, wchar_t** out_text)
{
    HANDLE file_handle;
    DWORD file_size;
    DWORD bytes_read;
    wchar_t* file_text;
    wchar_t* result_text;
    wchar_t* content_start;
    size_t content_length;

    if (path == NULL || out_text == NULL)
        return NOTE_STORE_READ_ERROR;

    *out_text = NULL;
    file_handle = CreateFileW(path,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return NOTE_STORE_READ_NOT_FOUND;
        return NOTE_STORE_READ_ERROR;
    }

    file_size = GetFileSize(file_handle, NULL);
    if (file_size == INVALID_FILE_SIZE)
    {
        CloseHandle(file_handle);
        return NOTE_STORE_READ_ERROR;
    }

    if (file_size == 0)
    {
        CloseHandle(file_handle);
        return NoteStore_AllocateEmptyText(out_text) == 0 ? NOTE_STORE_READ_OK : NOTE_STORE_READ_ERROR;
    }

    if ((file_size % sizeof(wchar_t)) != 0)
    {
        CloseHandle(file_handle);
        return NOTE_STORE_READ_INVALID;
    }

    file_text = (wchar_t*)malloc((size_t)file_size + sizeof(wchar_t));
    if (file_text == NULL)
    {
        CloseHandle(file_handle);
        return NOTE_STORE_READ_ERROR;
    }

    bytes_read = 0;
    if (!ReadFile(file_handle, file_text, file_size, &bytes_read, NULL) || bytes_read != file_size)
    {
        free(file_text);
        CloseHandle(file_handle);
        return NOTE_STORE_READ_ERROR;
    }

    CloseHandle(file_handle);
    file_text[file_size / sizeof(wchar_t)] = L'\0';
    content_start = file_text;
    if (content_start[0] == 0xFEFF)
        content_start += 1;

    content_length = wcslen(content_start);
    result_text = (wchar_t*)malloc((content_length + 1) * sizeof(wchar_t));
    if (result_text == NULL)
    {
        free(file_text);
        return NOTE_STORE_READ_ERROR;
    }

    memcpy(result_text, content_start, (content_length + 1) * sizeof(wchar_t));
    free(file_text);
    *out_text = result_text;
    return NOTE_STORE_READ_OK;
}

int NoteStore_GetDefaultNotePath(wchar_t* buffer, int buffer_count)
{
    wchar_t root_path[MAX_PATH];

    if (StateStore_GetRootPath(root_path, MAX_PATH) != 0)
        return 1;

    return NoteStore_CombinePath(buffer, buffer_count, root_path, NOTE_STORE_FILE_NAME);
}

int NoteStore_LoadText(const wchar_t* path, wchar_t** out_text)
{
    NoteStoreReadResult load_result;

    if (path == NULL || out_text == NULL)
        return 1;

    load_result = NoteStore_LoadUtf16File(path, out_text);
    if (load_result == NOTE_STORE_READ_NOT_FOUND)
        return NoteStore_AllocateEmptyText(out_text);
    return load_result == NOTE_STORE_READ_OK ? 0 : 1;
}

int NoteStore_SaveText(const wchar_t* path, const wchar_t* text)
{
    if (path == NULL || text == NULL)
        return 1;
    if (NoteStore_EnsureParentDirectoryExists(path) != 0)
        return 1;

    return StorageWriteUtf16FileAtomic(path, text);
}

NoteRecoveryLoadResult NoteStore_LoadRecoverySnapshot(const wchar_t* note_path,
                                                      NoteRecoverySnapshot* out_snapshot)
{
    NoteStoreReadResult load_result;
    wchar_t recovery_path[MAX_PATH];
    wchar_t* file_text;
    wchar_t* line_end;
    wchar_t* text_start;
    wchar_t* parse_end;
    wchar_t* snapshot_text;
    unsigned long version;
    size_t snapshot_text_length;

    if (note_path == NULL || out_snapshot == NULL)
        return NOTE_RECOVERY_LOAD_ERROR;
    if (NoteStore_BuildRecoveryPath(note_path, recovery_path, MAX_PATH) != 0)
        return NOTE_RECOVERY_LOAD_ERROR;

    out_snapshot->version = 0;
    out_snapshot->text = NULL;
    file_text = NULL;
    load_result = NoteStore_LoadUtf16File(recovery_path, &file_text);
    if (load_result == NOTE_STORE_READ_NOT_FOUND)
        return NOTE_RECOVERY_LOAD_NOT_FOUND;
    if (load_result == NOTE_STORE_READ_INVALID)
        return NOTE_RECOVERY_LOAD_INVALID;
    if (load_result != NOTE_STORE_READ_OK)
        return NOTE_RECOVERY_LOAD_ERROR;

    if (wcsncmp(file_text, L"version=", 8) != 0)
    {
        free(file_text);
        return NOTE_RECOVERY_LOAD_INVALID;
    }

    line_end = wcschr(file_text, L'\n');
    if (line_end != NULL)
    {
        *line_end = L'\0';
        text_start = line_end + 1;
    }
    else
    {
        text_start = file_text + wcslen(file_text);
    }

    if (text_start > file_text && text_start[-1] == L'\r')
        text_start[-1] = L'\0';

    version = wcstoul(file_text + 8, &parse_end, 10);
    if (parse_end == file_text + 8 || *parse_end != L'\0')
    {
        free(file_text);
        return NOTE_RECOVERY_LOAD_INVALID;
    }

    snapshot_text_length = wcslen(text_start);
    snapshot_text = (wchar_t*)malloc((snapshot_text_length + 1) * sizeof(wchar_t));
    if (snapshot_text == NULL)
    {
        free(file_text);
        return NOTE_RECOVERY_LOAD_ERROR;
    }

    memcpy(snapshot_text, text_start, (snapshot_text_length + 1) * sizeof(wchar_t));
    free(file_text);
    out_snapshot->version = (unsigned int)version;
    out_snapshot->text = snapshot_text;
    return NOTE_RECOVERY_LOAD_OK;
}

int NoteStore_SaveRecoverySnapshot(const wchar_t* note_path, unsigned int version, const wchar_t* text)
{
    wchar_t recovery_path[MAX_PATH];
    wchar_t version_text[64];
    wchar_t* snapshot_text;
    size_t version_text_length;
    size_t text_length;
    int written_characters;
    int result;

    if (note_path == NULL || text == NULL)
        return 1;
    if (NoteStore_BuildRecoveryPath(note_path, recovery_path, MAX_PATH) != 0)
        return 1;
    if (NoteStore_EnsureParentDirectoryExists(recovery_path) != 0)
        return 1;

    written_characters = swprintf(version_text,
                                  sizeof(version_text) / sizeof(version_text[0]),
                                  L"version=%u\n",
                                  version);
    if (written_characters < 0)
        return 1;

    version_text_length = wcslen(version_text);
    text_length = wcslen(text);
    snapshot_text = (wchar_t*)malloc((version_text_length + text_length + 1) * sizeof(wchar_t));
    if (snapshot_text == NULL)
        return 1;

    memcpy(snapshot_text, version_text, version_text_length * sizeof(wchar_t));
    memcpy(snapshot_text + version_text_length, text, (text_length + 1) * sizeof(wchar_t));
    result = StorageWriteUtf16FileAtomic(recovery_path, snapshot_text);
    free(snapshot_text);
    return result;
}

int NoteStore_DeleteRecoverySnapshot(const wchar_t* note_path)
{
    wchar_t recovery_path[MAX_PATH];

    if (note_path == NULL)
        return 1;
    if (NoteStore_BuildRecoveryPath(note_path, recovery_path, MAX_PATH) != 0)
        return 1;

    if (DeleteFileW(recovery_path))
        return 0;
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
        return 0;
    return 1;
}

void NoteStore_FreeText(wchar_t* text)
{
    free(text);
}

void NoteStore_FreeRecoverySnapshot(NoteRecoverySnapshot* snapshot)
{
    if (snapshot == NULL)
        return;

    free(snapshot->text);
    snapshot->text = NULL;
    snapshot->version = 0;
}
