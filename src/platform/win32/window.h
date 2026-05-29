#ifndef WINDOW_H
#define WINDOW_H

int Window_Run(void);

/* Platform API for frameless / client-area operations */
int Platform_SetFrameless(HWND hwnd, int enable);
int Platform_RebuildClientArea(HWND hwnd);
int Platform_ComputeContentRect(HWND hwnd, RECT* out_rect);

/* Platform API for window drag */
void Platform_BeginWindowDrag(HWND hwnd);

/* Shell-3a_2: 弹出托盘右键菜单 */
void Platform_ShowTrayMenu(HWND hwnd);

/* Shell-3a_1: 托盘自定义消息 */
#define WM_TRAYICON (WM_APP + 1)

#endif
