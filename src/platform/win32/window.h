#ifndef WINDOW_H
#define WINDOW_H

int Window_Run(void);

/* Platform API for frameless / client-area operations */
int Platform_SetFrameless(HWND hwnd, int enable);
int Platform_RebuildClientArea(HWND hwnd);
int Platform_ComputeContentRect(HWND hwnd, RECT* out_rect);

/* Platform API for window drag */
void Platform_BeginWindowDrag(HWND hwnd);

#endif
