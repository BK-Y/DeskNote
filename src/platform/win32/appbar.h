#ifndef DESKNOTE_APPBAR_H
#define DESKNOTE_APPBAR_H

#include <windows.h>

/* AppBar 贴边方向 */
typedef enum {
    APP_DOCK_LEFT = 0,
    APP_DOCK_RIGHT,
    APP_DOCK_TOP,
    APP_DOCK_BOTTOM
} AppDockEdge;

/* 注册当前窗口为 AppBar，返回 0 成功 */
int AppBar_Register(HWND hwnd);

/* 注销 AppBar，返回 0 成功 */
int AppBar_Unregister(HWND hwnd);

/* 设置贴边位置，传入 edge 和 thickness */
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness);

/* 获取当前 AppBar 是否已注册 */
int AppBar_IsRegistered(HWND hwnd);

/* 重注册 AppBar（Shell-5b），内部自行判断是否需要恢复 */
int AppBar_ReRegister(HWND hwnd);

/* 从 state.ini 读取 dock 配置（Shell-5b） */
int AppBar_ReadDockConfig(AppDockEdge* out_edge, int* out_thickness);

#endif
