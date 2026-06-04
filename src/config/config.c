#include "config.h"
#include <stddef.h>

static ConfigCallback g_cb = NULL;

int Config_Init(const char* path) { (void)path; return 0; }
int Config_Get(const char* key, int default_val) { (void)key; return default_val; }
int Config_Set(const char* key, int new_val) { (void)key; (void)new_val; return 0; }
void Config_OnChange(ConfigCallback cb) { g_cb = cb; }
void Config_Shutdown(void) {}
