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
    int presence_state;              /* Shell-3c_2: 0=visible_front, 1=hidden_to_tray, 2=exiting */
    int last_floating_left;          /* Shell-4a_2: 上次浮动置顶时的窗口位置 */
    int last_floating_top;
    int last_floating_width;
    int last_floating_height;
} StateData;

int StateStore_GetRootPath(wchar_t* buffer, int buffer_count);
int StateStore_GetStatePath(wchar_t* buffer, int buffer_count);
int StateStore_Load(StateData* out_state);
int StateStore_Save(const StateData* state);

#endif
