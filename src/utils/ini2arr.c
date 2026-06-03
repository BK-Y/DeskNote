#include "ini2arr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUF_SIZE 256
#define INITIAL_CAP   16

int ini2arr(const char* path, Ini2Arr* out)
{
    FILE* f = fopen(path, "r");
    if (!f) return 1;

    int capacity = INITIAL_CAP;
    out->entries = (Ini2ArrEntry*)malloc(sizeof(Ini2ArrEntry) * capacity);
    out->count   = 0;

    char buf[LINE_BUF_SIZE];
    while (fgets(buf, sizeof(buf), f))
    {
        char key[INI2ARR_KEY_LEN + 1];
        int  val;

        if (sscanf(buf, " %63[^=]=%d", key, &val) != 2)
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
                fclose(f);
                return 1;
            }
            out->entries = newp;
        }

        strncpy(out->entries[out->count].key, key, INI2ARR_KEY_LEN);
        out->entries[out->count].key[INI2ARR_KEY_LEN] = '\0';
        out->entries[out->count].val = val;
        out->count++;
    }

    fclose(f);
    return 0;
}

int ini2arr_write(const char* path, const char* key, int val)
{
    FILE* src = fopen(path, "r");
    if (!src) return 1;

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE* dst = fopen(tmp_path, "w");
    if (!dst)
    {
        fclose(src);
        return 1;
    }

    char  line[LINE_BUF_SIZE];
    int   found = 0;

    while (fgets(line, sizeof(line), src))
    {
        char k[INI2ARR_KEY_LEN + 1];
        int  v;

        if (sscanf(line, " %63[^=]=%d", k, &v) == 2 &&
            strcmp(k, key) == 0)
        {
            fprintf(dst, "%s=%d\n", key, val);
            found = 1;
        }
        else
        {
            fputs(line, dst);
        }
    }

    if (!found)
        fprintf(dst, "%s=%d\n", key, val);

    fclose(src);
    fclose(dst);

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
