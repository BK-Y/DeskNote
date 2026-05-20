// notefile.c - 便签文件读写
#include "notefile.h"
#include "storage.h"
#include <wchar.h>

static wchar_t s_currentPath[MAX_PATH];
static wchar_t s_currentName[256];
static int s_hasFile = 0;

static wchar_t* ReadFileContent(const wchar_t* path)
{
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD size = GetFileSize(hFile, NULL);
    if (size == 0 || size > 65536) { CloseHandle(hFile); return NULL; }

    // 读文件为 UTF-8 字节
    char* utf8Buf = (char*)malloc(size + 1);
    if (!utf8Buf) { CloseHandle(hFile); return NULL; }

    DWORD read = 0;
    ReadFile(hFile, utf8Buf, size, &read, NULL);
    utf8Buf[read] = '\0';
    CloseHandle(hFile);

    // 跳过 BOM（如果有）
    char* start = utf8Buf;
    if (size >= 3 && (unsigned char)utf8Buf[0] == 0xEF &&
                     (unsigned char)utf8Buf[1] == 0xBB &&
                     (unsigned char)utf8Buf[2] == 0xBF)
    {
        start = utf8Buf + 3;
    }

    // UTF-8 → wchar_t 转换
    int wlen = MultiByteToWideChar(CP_UTF8, 0, start, -1, NULL, 0);
    wchar_t* content = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (content)
        MultiByteToWideChar(CP_UTF8, 0, start, -1, content, wlen);

    free(utf8Buf);
    return content;
}

static int WriteFileContent(const wchar_t* path, const wchar_t* content)
{
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 1;

    // 写入 UTF-8 BOM
    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    DWORD written = 0;
    WriteFile(hFile, bom, 3, &written, NULL);

    // wchar_t (UTF-16) → UTF-8 转换
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, content, -1, NULL, 0, NULL, NULL);
    if (utf8Len > 0)
    {
        char* utf8Buf = (char*)malloc(utf8Len);
        if (utf8Buf)
        {
            WideCharToMultiByte(CP_UTF8, 0, content, -1, utf8Buf, utf8Len, NULL, NULL);
            // 不写入 null 终止符（-1 已包含，但我们要去掉）
            WriteFile(hFile, utf8Buf, utf8Len - 1, &written, NULL);
            free(utf8Buf);
        }
    }

    CloseHandle(hFile);
    return 0;
}

// 生成当日第一个可用文件名，格式 dn_YYYYMMDD_###.md
static void GenerateNewName(wchar_t* nameBuf, int bufsize)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t dateStr[16];
    wsprintfW(dateStr, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);

    wchar_t prefix[32];
    wsprintfW(prefix, L"dn_%ls_", dateStr);

    // 扫描目录，找今日已有文件的最大序号
    const wchar_t* notesDir = Storage_GetNotesDir();
    wchar_t pattern[MAX_PATH];
    wcscpy(pattern, notesDir);
    wcscat(pattern, L"\\dn_*");

    int maxSeq = 0;
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pattern, &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            // 检查文件名是否以今日前缀开头
            if (wcsncmp(ffd.cFileName, prefix, wcslen(prefix)) == 0)
            {
                // 提取序号部分（前缀后的三位数字）
                const wchar_t* seqPart = ffd.cFileName + wcslen(prefix);
                int seq = 0;
                for (int i = 0; i < 3 && seqPart[i] >= L'0' && seqPart[i] <= L'9'; i++)
                    seq = seq * 10 + (seqPart[i] - L'0');
                if (seq > maxSeq) maxSeq = seq;
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }

    wsprintfW(nameBuf, L"dn_%ls_%03d.md", dateStr, maxSeq + 1);
}

int NoteFile_OpenLast(HWND hEditor)
{
    const wchar_t* notesDir = Storage_GetNotesDir();

    // 尝试恢复上次打开的文件
    wchar_t lastName[256];
    int hasLast = Storage_ReadSetting(L"last_file", lastName, 256);

    if (hasLast > 0)
    {
        wcscpy(s_currentPath, notesDir);
        wcscat(s_currentPath, L"\\");
        wcscat(s_currentPath, lastName);

        DWORD attr = GetFileAttributesW(s_currentPath);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            wchar_t* content = ReadFileContent(s_currentPath);
            if (content)
            {
                SetWindowTextW(hEditor, content);
                free(content);
            }
            wcscpy(s_currentName, lastName);
            s_hasFile = 1;
            return 0;
        }
    }

    // 无记录或文件已删除，创建新文件
    GenerateNewName(s_currentName, 256);
    wcscpy(s_currentPath, notesDir);
    wcscat(s_currentPath, L"\\");
    wcscat(s_currentPath, s_currentName);
    WriteFileContent(s_currentPath, L"");
    s_hasFile = 1;
    return 0;
}

void NoteFile_Save(HWND hEditor)
{
    if (!s_hasFile || !hEditor) return;

    int len = GetWindowTextLengthW(hEditor);
    if (len == 0) { WriteFileContent(s_currentPath, L""); return; }

    wchar_t* content = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (!content) return;

    GetWindowTextW(hEditor, content, len + 1);
    WriteFileContent(s_currentPath, content);
    free(content);
}

void NoteFile_Close(HWND hEditor)
{
    if (!s_hasFile) return;
    NoteFile_Save(hEditor);
    Storage_WriteSetting(L"last_file", s_currentName);
    s_hasFile = 0;
}

const wchar_t* NoteFile_GetName(void)
{
    return s_hasFile ? s_currentName : L"";
}
