#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <windows.h>

// 标题栏高度
#define TITLEBAR_HEIGHT 30

// 按钮 ID
#define TITLEBAR_BTN_CLOSE  1
#define TITLEBAR_BTN_PIN    2
#define TITLEBAR_BTN_HIDE   3

// 绘制标题栏（在 WM_PAINT 中调用）
void Titlebar_Draw(HWND hwnd, HDC hdc);

// 检测点击位置（在 WM_LBUTTONDOWN 中调用）
// 返回值：0 = 内容区, 按钮 ID = 对应按钮, -1 = 拖拽区
int Titlebar_HitTest(HWND hwnd, int x, int y);

// 鼠标移动时调用（更新悬停状态）
void Titlebar_OnMouseMove(HWND hwnd, int x, int y);
// 鼠标离开窗口时调用（重置悬停状态）
void Titlebar_OnMouseLeave(HWND hwnd);

// 置顶切换
void Titlebar_TogglePin(HWND hwnd);

#endif
