// storage.c - 文件路径 + 配置读写
#include "storage.h"
#include <wchar.h>

#define APP_NAME    L"DeskNote"
#define NOTES_DIR   L"notes"
#define SETTINGS_FILE L"settings.json"

static wchar_t s_appDir[MAX_PATH];
static wchar_t s_notesDir[MAX_PATH];
static wchar_t s_settingsPath[MAX_PATH];
static int s_inited = 0;

static int EnsureDirExists(const wchar_t* path)
{
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return CreateDirectoryW(path, NULL) ? 0 : 1;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
}

int Storage_Init(void)
{
    if (s_inited) return 0;

    wchar_t env[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"APPDATA", env, MAX_PATH);
    if (len == 0 || len > MAX_PATH) return 1;

    wcscpy(s_appDir, env);
    wcscat(s_appDir, L"\\");
    wcscat(s_appDir, APP_NAME);

    wcscpy(s_notesDir, s_appDir);
    wcscat(s_notesDir, L"\\");
    wcscat(s_notesDir, NOTES_DIR);

    wcscpy(s_settingsPath, s_appDir);
    wcscat(s_settingsPath, L"\\");
    wcscat(s_settingsPath, SETTINGS_FILE);

    if (EnsureDirExists(s_appDir))   return 1;
    if (EnsureDirExists(s_notesDir)) return 1;

    s_inited = 1;
    return 0;
}

const wchar_t* Storage_GetNotesDir(void)
{
    return s_notesDir;
}

const wchar_t* Storage_GetSettingsPath(void)
{
    return s_settingsPath;
}

static int FindKey(const wchar_t* json, const wchar_t* key)
{
    int keyLen = (int)wcslen(key);
    int jsonLen = (int)wcslen(json);
    int i;
    for (i = 0; i <= jsonLen - keyLen - 4; i++)
    {
        if (json[i] == L'"' &&
            wcsncmp(&json[i + 1], key, keyLen) == 0 &&
            json[i + 1 + keyLen] == L'"' &&
            json[i + 2 + keyLen] == L':')
        {
            // 跳过冒号后的空白
            int valPos = i + 3 + keyLen;
            while (json[valPos] == L' ' || json[valPos] == L'\t' ||
                   json[valPos] == L'\n' || json[valPos] == L'\r')
                valPos++;
            if (json[valPos] == L'"')
                return valPos + 1;
        }
    }
    return -1;
}

int Storage_ReadSetting(const wchar_t* key, wchar_t* buf, int bufsize)
{
    if (!s_inited || !buf || bufsize <= 0) return 0;

    HANDLE hFile = CreateFileW(s_settingsPath, GENERIC_READ,
                                FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 65536)
    {
        CloseHandle(hFile);
        return 0;
    }

    wchar_t* content = (wchar_t*)malloc(fileSize + sizeof(wchar_t));
    if (!content) { CloseHandle(hFile); return 0; }

    DWORD read = 0;
    ReadFile(hFile, content, fileSize, &read, NULL);
    content[read / sizeof(wchar_t)] = L'\0';
    CloseHandle(hFile);

    int valStart = FindKey(content, key);
    int result = 0;

    if (valStart >= 0)
    {
        int j;
        for (j = 0; content[valStart + j] != L'"' && j < bufsize - 1; j++)
            buf[j] = content[valStart + j];
        buf[j] = L'\0';
        result = j;
    }

    free(content);
    return result;
}

int Storage_WriteSetting(const wchar_t* key, const wchar_t* value)
{
    if (!s_inited) return 1;

    wchar_t newJson[MAX_PATH * 2];
    int n = swprintf(newJson, MAX_PATH * 2,
                     L"{\n  \"%ls\": \"%ls\"\n}", key, value);
    if (n < 0) return 1;

    HANDLE hFile = CreateFileW(s_settingsPath, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 1;

    DWORD bytesToWrite = (DWORD)(wcslen(newJson) * sizeof(wchar_t));
    DWORD written = 0;
    WriteFile(hFile, newJson, bytesToWrite, &written, NULL);
    CloseHandle(hFile);

    return (written == bytesToWrite) ? 0 : 1;
}
