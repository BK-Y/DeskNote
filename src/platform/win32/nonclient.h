#ifndef DESKNOTE_NONCLIENT_H
#define DESKNOTE_NONCLIENT_H

#include <windows.h>

int Platform_Nonclient_Init(HWND hwnd);
int Platform_Nonclient_SetMetrics(HWND hwnd,
                                  int enabled,
                                  int titlebar_height,
                                  int frame_visual_thickness,
                                  int frame_resize_thickness);
int Platform_Nonclient_IsEnabled(void);
int Platform_Nonclient_ComputeContentRect(HWND hwnd, RECT* out_rect);
LRESULT Platform_Nonclient_HandleNCHitTest(HWND hwnd, LPARAM lParam);
void Platform_Nonclient_Shutdown(HWND hwnd);

#endif
