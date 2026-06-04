#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_KEY_LEN 63

typedef void (*ConfigCallback)(const char* key, int old_val, int new_val);

int  Config_Init(const char* path);
int  Config_Get(const char* key, int default_val);
int  Config_Set(const char* key, int new_val);
void Config_OnChange(ConfigCallback cb);
void Config_Shutdown(void);

#endif
