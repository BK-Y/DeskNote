#include "storage_write.h"

#include <string.h>
#include <wchar.h>

static int StorageWrite_CopyString(wchar_t* buffer, int buffer_count, const wchar_t* text)
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

static int StorageWrite_BuildTempPath(const wchar_t* path, wchar_t* temp_path, int temp_path_count)
{
    static const wchar_t suffix[] = L".tmp";
    size_t path_length;
    size_t suffix_length;

    if (path == NULL || temp_path == NULL || temp_path_count <= 0)
        return 1;

    path_length = wcslen(path);
    suffix_length = wcslen(suffix);
    if ((int)(path_length + suffix_length) >= temp_path_count)
        return 1;

    if (StorageWrite_CopyString(temp_path, temp_path_count, path) != 0)
        return 1;

    memcpy(temp_path + path_length, suffix, (suffix_length + 1) * sizeof(wchar_t));
    return 0;
}

int StorageWriteUtf16FileAtomic(const wchar_t* path, const wchar_t* text)
{
    HANDLE file_handle;
    wchar_t temp_path[MAX_PATH];
    DWORD bytes_to_write;
    DWORD bytes_written;
    int result;
    static const unsigned char utf16le_bom[] = { 0xFF, 0xFE };

    if (path == NULL || text == NULL)
        return 1;
    if (StorageWrite_BuildTempPath(path, temp_path, MAX_PATH) != 0)
        return 1;

    file_handle = CreateFileW(temp_path,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 1;

    result = 1;
    bytes_written = 0;
    if (!WriteFile(file_handle, utf16le_bom, sizeof(utf16le_bom), &bytes_written, NULL) ||
        bytes_written != sizeof(utf16le_bom))
    {
        CloseHandle(file_handle);
        DeleteFileW(temp_path);
        return 1;
    }

    bytes_to_write = (DWORD)(wcslen(text) * sizeof(wchar_t));
    bytes_written = 0;
    if (!WriteFile(file_handle, text, bytes_to_write, &bytes_written, NULL) ||
        bytes_written != bytes_to_write)
    {
        CloseHandle(file_handle);
        DeleteFileW(temp_path);
        return 1;
    }

    if (!FlushFileBuffers(file_handle))
    {
        CloseHandle(file_handle);
        DeleteFileW(temp_path);
        return 1;
    }

    if (CloseHandle(file_handle))
    {
        file_handle = INVALID_HANDLE_VALUE;
        if (MoveFileExW(temp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            result = 0;
    }

    if (file_handle != INVALID_HANDLE_VALUE)
        CloseHandle(file_handle);
    if (result != 0)
        DeleteFileW(temp_path);
    return result;
}
