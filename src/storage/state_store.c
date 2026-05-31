#include "state_store.h"
#include "storage_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define STATE_STORE_VERSION 2
#define STATE_STORE_ROOT_NAME L"DeskNote"
#define STATE_STORE_FILE_NAME L"state.ini"

static int StateStore_CopyString(wchar_t* buffer, int buffer_count, const wchar_t* text)
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

static int StateStore_CombinePath(wchar_t* buffer,
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

static int StateStore_EnsureDirectoryExists(const wchar_t* path)
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

static void StateStore_TrimLineEndings(wchar_t* text)
{
    size_t length;

    if (text == NULL)
        return;

    length = wcslen(text);
    while (length > 0 && (text[length - 1] == L'\r' || text[length - 1] == L'\n'))
    {
        text[length - 1] = L'\0';
        length -= 1;
    }
}

int StateStore_GetRootPath(wchar_t* buffer, int buffer_count)
{
    wchar_t local_app_data[MAX_PATH];
    DWORD length;

    if (buffer == NULL || buffer_count <= 0)
        return 1;

    length = GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return 1;

    return StateStore_CombinePath(buffer, buffer_count, local_app_data, STATE_STORE_ROOT_NAME);
}

int StateStore_GetStatePath(wchar_t* buffer, int buffer_count)
{
    wchar_t root_path[MAX_PATH];

    if (StateStore_GetRootPath(root_path, MAX_PATH) != 0)
        return 1;

    return StateStore_CombinePath(buffer, buffer_count, root_path, STATE_STORE_FILE_NAME);
}

int StateStore_Load(StateData* out_state)
{
    HANDLE file_handle;
    wchar_t state_path[MAX_PATH];
    wchar_t* file_text;
    wchar_t* parse_cursor;
    DWORD bytes_read;
    DWORD file_size;

    if (out_state == NULL)
        return 1;

    memset(out_state, 0, sizeof(*out_state));
    out_state->version = STATE_STORE_VERSION;
    out_state->use_custom_chrome = 1;

    if (StateStore_GetStatePath(state_path, MAX_PATH) != 0)
        return 1;

    file_handle = CreateFileW(state_path,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return 0;
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
        return 0;
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
    parse_cursor = file_text;
    if (parse_cursor[0] == 0xFEFF)
        parse_cursor += 1;

    while (*parse_cursor != L'\0')
    {
        wchar_t* line_start;
        wchar_t* line_end;

        line_start = parse_cursor;
        while (*parse_cursor != L'\0' && *parse_cursor != L'\n')
            parse_cursor += 1;

        line_end = parse_cursor;
        if (*parse_cursor == L'\n')
        {
            *parse_cursor = L'\0';
            parse_cursor += 1;
        }

        StateStore_TrimLineEndings(line_start);
        if (wcsncmp(line_start, L"version=", 8) == 0)
        {
            out_state->version = _wtoi(line_start + 8);
        }
        else if (wcsncmp(line_start, L"last_file=", 10) == 0)
        {
            if (line_start[10] != L'\0' &&
                StateStore_CopyString(out_state->last_file, MAX_PATH, line_start + 10) == 0)
            {
                out_state->has_last_file = 1;
            }
        }
        else if (wcsncmp(line_start, L"use_custom_chrome=", 18) == 0)
        {
            out_state->use_custom_chrome = _wtoi(line_start + 18) != 0 ? 1 : 0;
        }
        else if (wcsncmp(line_start, L"titlebar_height=", 16) == 0)
        {
            out_state->titlebar_height = _wtoi(line_start + 16);
        }
        else if (wcsncmp(line_start, L"frame_visual_thickness=", 23) == 0)
        {
            out_state->frame_visual_thickness = _wtoi(line_start + 23);
        }
        else if (wcsncmp(line_start, L"shell_resident_mode=", 20) == 0)
        {
            out_state->shell_resident_mode = _wtoi(line_start + 20);
        }
        else if (wcsncmp(line_start, L"presence_state=", 15) == 0)
        {
            out_state->presence_state = _wtoi(line_start + 15);
        }
        else if (wcsncmp(line_start, L"last_floating_left=", 19) == 0)
        {
            out_state->last_floating_left = _wtoi(line_start + 19);
        }
        else if (wcsncmp(line_start, L"last_floating_top=", 18) == 0)
        {
            out_state->last_floating_top = _wtoi(line_start + 18);
        }
        else if (wcsncmp(line_start, L"last_floating_width=", 20) == 0)
        {
            out_state->last_floating_width = _wtoi(line_start + 20);
        }
        else if (wcsncmp(line_start, L"last_floating_height=", 21) == 0)
        {
            out_state->last_floating_height = _wtoi(line_start + 21);
        }
        else if (wcsncmp(line_start, L"dock_edge=", 10) == 0)
        {
            out_state->dock_edge = _wtoi(line_start + 10);
        }
        else if (wcsncmp(line_start, L"dock_thickness=", 15) == 0)
        {
            out_state->dock_thickness = _wtoi(line_start + 15);
        }

        if (line_end == parse_cursor)
            break;
    }

    free(file_text);
    return 0;
}

int StateStore_Save(const StateData* state)
{
    wchar_t root_path[MAX_PATH];
    wchar_t state_path[MAX_PATH];
    wchar_t file_text[MAX_PATH + 192];
    int written_characters;

    if (state == NULL)
        return 1;

    if (StateStore_GetRootPath(root_path, MAX_PATH) != 0)
        return 1;
    if (StateStore_GetStatePath(state_path, MAX_PATH) != 0)
        return 1;
    if (StateStore_EnsureDirectoryExists(root_path) != 0)
        return 1;

    written_characters = swprintf(file_text,
                                  sizeof(file_text) / sizeof(file_text[0]),
                                  L"version=%d\r\n"
                                  L"last_file=%ls\r\n"
                                  L"use_custom_chrome=%d\r\n"
                                  L"titlebar_height=%d\r\n"
                                  L"frame_visual_thickness=%d\r\n"
                                  L"shell_resident_mode=%d\r\n"
                                  L"presence_state=%d\r\n"
                                  L"last_floating_left=%d\r\n"
                                  L"last_floating_top=%d\r\n"
                                  L"last_floating_width=%d\r\n"
                                  L"last_floating_height=%d\r\n"
                                  L"dock_edge=%d\r\n"
                                  L"dock_thickness=%d\r\n",
                                  STATE_STORE_VERSION,
                                  state->has_last_file ? state->last_file : L"",
                                  state->use_custom_chrome,
                                  state->titlebar_height,
                                  state->frame_visual_thickness,
                                  state->shell_resident_mode,
                                  state->presence_state,
                                  state->last_floating_left,
                                  state->last_floating_top,
                                  state->last_floating_width,
                                  state->last_floating_height,
                                  state->dock_edge,
                                  state->dock_thickness);
    if (written_characters < 0)
        return 1;

    return StorageWriteUtf16FileAtomic(state_path, file_text);
}
