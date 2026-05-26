#include "note_store.h"

#include "state_store.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define NOTE_STORE_FILE_NAME L"note.md"

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

int NoteStore_GetDefaultNotePath(wchar_t* buffer, int buffer_count)
{
    wchar_t root_path[MAX_PATH];

    if (StateStore_GetRootPath(root_path, MAX_PATH) != 0)
        return 1;

    return NoteStore_CombinePath(buffer, buffer_count, root_path, NOTE_STORE_FILE_NAME);
}

int NoteStore_LoadText(const wchar_t* path, wchar_t** out_text)
{
    HANDLE file_handle;
    DWORD file_size;
    DWORD bytes_read;
    wchar_t* file_text;
    wchar_t* result_text;
    wchar_t* content_start;
    size_t content_length;

    if (path == NULL || out_text == NULL)
        return 1;

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
            return NoteStore_AllocateEmptyText(out_text);
        return 1;
    }

    file_size = GetFileSize(file_handle, NULL);
    if (file_size == INVALID_FILE_SIZE)
    {
        CloseHandle(file_handle);
        return 1;
    }

    if (file_size == 0)
    {
        CloseHandle(file_handle);
        return NoteStore_AllocateEmptyText(out_text);
    }

    if ((file_size % sizeof(wchar_t)) != 0)
    {
        CloseHandle(file_handle);
        return 1;
    }

    file_text = (wchar_t*)malloc((size_t)file_size + sizeof(wchar_t));
    if (file_text == NULL)
    {
        CloseHandle(file_handle);
        return 1;
    }

    bytes_read = 0;
    if (!ReadFile(file_handle, file_text, file_size, &bytes_read, NULL) || bytes_read != file_size)
    {
        free(file_text);
        CloseHandle(file_handle);
        return 1;
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
        return 1;
    }

    memcpy(result_text, content_start, (content_length + 1) * sizeof(wchar_t));
    free(file_text);
    *out_text = result_text;
    return 0;
}

int NoteStore_SaveText(const wchar_t* path, const wchar_t* text)
{
    HANDLE file_handle;
    DWORD bytes_to_write;
    DWORD bytes_written;
    static const unsigned char utf16le_bom[] = { 0xFF, 0xFE };

    if (path == NULL || text == NULL)
        return 1;
    if (NoteStore_EnsureParentDirectoryExists(path) != 0)
        return 1;

    file_handle = CreateFileW(path,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 1;

    bytes_written = 0;
    if (!WriteFile(file_handle, utf16le_bom, sizeof(utf16le_bom), &bytes_written, NULL) ||
        bytes_written != sizeof(utf16le_bom))
    {
        CloseHandle(file_handle);
        return 1;
    }

    bytes_to_write = (DWORD)(wcslen(text) * sizeof(wchar_t));
    if (!WriteFile(file_handle, text, bytes_to_write, &bytes_written, NULL) ||
        bytes_written != bytes_to_write)
    {
        CloseHandle(file_handle);
        return 1;
    }

    CloseHandle(file_handle);
    return 0;
}

void NoteStore_FreeText(wchar_t* text)
{
    free(text);
}
