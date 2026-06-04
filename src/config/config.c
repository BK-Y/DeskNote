#include "config.h"
#include "../utils/ini2arr.h"
#include <string.h>

static struct {
    Ini2Arr arr;
    char    path[512];
    ConfigCallback cb;
} g_config;

#define ARR (&g_config.arr)

int Config_Init(const char* path)
{
    if (ARR->entries)
        ini2arr_free(ARR);
    strncpy(g_config.path, path, sizeof(g_config.path) - 1);
    return ini2arr(path, ARR);
}

int Config_Get(const char* key, int default_val)
{
    for (int i = 0; i < ARR->count; i++)
        if (strcmp(ARR->entries[i].key, key) == 0)
            return ARR->entries[i].val;
    return default_val;
}

int Config_Set(const char* key, int new_val)
{
    if (!ARR->entries)
        return 1;

    int old_val = Config_Get(key, -1);
    if (old_val == new_val)
        return 0;

    for (int i = 0; i < ARR->count; i++)
        if (strcmp(ARR->entries[i].key, key) == 0)
        { ARR->entries[i].val = new_val; goto write; }

    strncpy(ARR->entries[ARR->count].key, key, INI2ARR_KEY_LEN);
    ARR->entries[ARR->count].val = new_val;
    ARR->count++;

write:
    if (ini2arr_write(g_config.path, key, new_val) != 0)
        return 1;

    if (g_config.cb)
        g_config.cb(key, old_val, new_val);

    return 0;
}

void Config_Shutdown(void)
{
    if (ARR->entries)
        ini2arr_free(ARR);
}

void Config_OnChange(ConfigCallback cb)
{
    g_config.cb = cb;
}
