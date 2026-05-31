#ifndef DESKNOTE_APPBAR_H
#define DESKNOTE_APPBAR_H

#include <windows.h>

/* AppBar 贴边方向（必须与 Windows ABE_* 常量值一致） */
typedef enum {
    APP_DOCK_LEFT   = 0,  /* ABE_LEFT   */
    APP_DOCK_TOP    = 1,  /* ABE_TOP    */
    APP_DOCK_RIGHT  = 2,  /* ABE_RIGHT  */
    APP_DOCK_BOTTOM = 3   /* ABE_BOTTOM */
} AppDockEdge;

/* 注册当前窗口为 AppBar，返回 0 成功（edge 和 thickness 在 ABM_NEW 前传入，避免空矩形注册） */
int AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness);

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
