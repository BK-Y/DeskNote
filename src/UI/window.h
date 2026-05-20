#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

// 右下角 resize 检测
int Window_ResizeHitTest(HWND hwnd, int x, int y);

// 窗口拖拽/大小改变结束时调用（接 WM_EXITSIZEMOVE）
void Window_CheckAndAutoHide(HWND hwnd);

// 定时器中检测鼠标是否靠近隐藏条
void Window_OnHideTimer(HWND hwnd);

// 手动弹出隐藏的窗口
void Window_PopOut(HWND hwnd);

// 查询是否处于隐藏状态
int  Window_IsHidden(void);

// 切换隐藏/弹出（供 🗕 按钮调用）
void Window_ToggleHide(HWND hwnd);

#endif
