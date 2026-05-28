#ifndef DESKNOTE_STATE_STORE_H
#define DESKNOTE_STATE_STORE_H

#include <windows.h>

typedef struct {
    int version;
    int has_last_file;
    wchar_t last_file[MAX_PATH];
    int use_custom_chrome;
    int titlebar_height;
    int frame_visual_thickness;
    int shell_resident_mode;
} StateData;

int StateStore_GetRootPath(wchar_t* buffer, int buffer_count);
int StateStore_GetStatePath(wchar_t* buffer, int buffer_count);
int StateStore_Load(StateData* out_state);
int StateStore_Save(const StateData* state);

#endif
