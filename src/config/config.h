#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_KEY_LEN 63

/* 默认值 */
#define CONFIG_DEFAULT_TITLEBAR_HEIGHT        32
#define CONFIG_DEFAULT_FRAME_VISUAL_THICKNESS   1
#define CONFIG_DEFAULT_FRAME_RESIZE_THICKNESS   6
#define CONFIG_DEFAULT_DOCK_EDGE                2
#define CONFIG_DEFAULT_DOCK_THICKNESS         240
#define CONFIG_DEFAULT_WINDOW_WIDTH           240
#define CONFIG_DEFAULT_WINDOW_HEIGHT          388

typedef void (*ConfigCallback)(const char* key, int old_val, int new_val);

int  Config_Init(const char* path);
int  Config_Get(const char* key, int default_val);
int  Config_Set(const char* key, int new_val);
void Config_OnChange(ConfigCallback cb);
void Config_Shutdown(void);

#endif
