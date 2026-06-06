#include "ini2arr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUF_SIZE 256
#define INITIAL_CAP   16

/* Read entire file into a heap-allocated ASCII buffer.
 * Detects UTF-16 LE BOM (FF FE) and converts to ASCII by discarding
 * the high-order null byte from each wchar_t pair.
 * Returns heap buffer (caller must free) or NULL on error.
 * Sets *out_len to the ASCII content length (excluding null terminator). */
static char* read_file_ascii(const char* path, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long raw_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (raw_size <= 0) { fclose(f); return NULL; }

    unsigned char* raw = (unsigned char*)malloc((size_t)raw_size);
    if (!raw) { fclose(f); return NULL; }
    if (fread(raw, 1, (size_t)raw_size, f) != (size_t)raw_size)
    { free(raw); fclose(f); return NULL; }
    fclose(f);

    int is_utf16le = 0;
    size_t start   = 0;
    if (raw_size >= 2 && raw[0] == 0xFF && raw[1] == 0xFE)
    { is_utf16le = 1; start = 2; }

    /* Convert: UTF-16 LE discards every other byte; ASCII passthrough */
    size_t ascii_cap = (size_t)raw_size + 1;
    char*  ascii     = (char*)malloc(ascii_cap);
    if (!ascii) { free(raw); return NULL; }

    size_t w = 0;
    for (size_t i = start; i < (size_t)raw_size; i++)
    {
        unsigned char c = raw[i];
        if (is_utf16le)
        {
            /* Skip high byte of each UTF-16 LE code unit (odd positions) */
            if ((i - start) % 2 == 1) continue;
            /* Handle \r\0\n\0 → single \n */
            if (c == '\r' && i + 2 < (size_t)raw_size && raw[i+1] == 0 && raw[i+2] == '\n')
            { ascii[w++] = '\n'; i += 2; continue; }
            if (c == '\r' && i + 2 < (size_t)raw_size && raw[i+1] == 0 && raw[i+2] == 0)
            { ascii[w++] = '\n'; i += 2; continue; }
        }
        ascii[w++] = (char)c;
    }
    ascii[w] = '\0';
    *out_len = w;
    free(raw);
    return ascii;
}

int ini2arr(const char* path, Ini2Arr* out)
{
    size_t len;
    char*  text = read_file_ascii(path, &len);
    if (!text) return 1;

    int capacity = INITIAL_CAP;
    out->entries = (Ini2ArrEntry*)malloc(sizeof(Ini2ArrEntry) * capacity);
    out->count   = 0;

    const char* p = text;
    const char* end = text + len;
    while (p < end)
    {
        /* Extract one line */
        const char* line_start = p;
        while (p < end && *p != '\n' && *p != '\r') p++;
        size_t line_len = (size_t)(p - line_start);
        /* Skip line terminators */
        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;

        if (line_len == 0 || line_len >= LINE_BUF_SIZE) continue;

        char key[INI2ARR_KEY_LEN + 1];
        int  val;
        char line_buf[LINE_BUF_SIZE];
        memcpy(line_buf, line_start, line_len);
        line_buf[line_len] = '\0';

        if (sscanf(line_buf, " %63[^=]=%d", key, &val) != 2)
            continue;

        if (out->count >= capacity)
        {
            capacity *= 2;
            Ini2ArrEntry* newp = (Ini2ArrEntry*)realloc(
                out->entries, sizeof(Ini2ArrEntry) * capacity);
            if (!newp)
            {
                free(out->entries);
                out->entries = NULL;
                out->count   = 0;
                free(text);
                return 1;
            }
            out->entries = newp;
        }

        strncpy(out->entries[out->count].key, key, INI2ARR_KEY_LEN);
        out->entries[out->count].key[INI2ARR_KEY_LEN] = '\0';
        out->entries[out->count].val = val;
        out->count++;
    }

    free(text);
    return 0;
}

int ini2arr_write(const char* path, const char* key, int val)
{
    size_t len;
    char*  text = read_file_ascii(path, &len);
    if (!text) return 1;

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE* dst = fopen(tmp_path, "w");
    if (!dst) { free(text); return 1; }

    int   found = 0;
    const char* p   = text;
    const char* end = text + len;

    while (p < end)
    {
        const char* line_start = p;
        while (p < end && *p != '\n' && *p != '\r') p++;
        size_t line_len = (size_t)(p - line_start);
        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;

        if (line_len == 0)
        { fputc('\n', dst); continue; }

        char k[INI2ARR_KEY_LEN + 1];
        int  v;
        char line_buf[LINE_BUF_SIZE];
        memcpy(line_buf, line_start, line_len);
        line_buf[line_len] = '\0';

        if (sscanf(line_buf, " %63[^=]=%d", k, &v) == 2 &&
            strcmp(k, key) == 0)
        {
            fprintf(dst, "%s=%d\n", key, val);
            found = 1;
        }
        else
        {
            fwrite(line_start, 1, line_len, dst);
            fputc('\n', dst);
        }
    }

    if (!found)
        fprintf(dst, "%s=%d\n", key, val);

    fclose(dst);
    free(text);

    remove(path);
    rename(tmp_path, path);

    return 0;
}

void ini2arr_free(Ini2Arr* arr)
{
    if (arr && arr->entries)
    {
        free(arr->entries);
        arr->entries = NULL;
        arr->count   = 0;
    }
}
