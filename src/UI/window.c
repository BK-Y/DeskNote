// window.c - 窗口框架交互（resize、靠边隐藏）
#include "window.h"
#include "../ui/titlebar.h"  // 用于 TITLEBAR_HEIGHT（隐藏时保留标题栏色条）

#define EDGE_THRESHOLD  20    // 拖到距边缘多近时触发隐藏
#define HIDE_EDGE       3     // 隐藏后保留的像素宽度（色条）
#define HIDE_MARGIN     30    // 鼠标靠近多少像素时弹出
#define IDT_HIDE        2001

enum { HIDE_NONE, HIDE_LEFT, HIDE_RIGHT };
static int s_hideSide = HIDE_NONE;

// ============================================================
// 右下角 resize
// ============================================================

int Window_ResizeHitTest(HWND hwnd, int x, int y)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    const int EDGE = 15;
    if (x >= rect.right - EDGE && y >= rect.bottom - EDGE)
        return HTBOTTOMRIGHT;

    return 0;
}

// ============================================================
// 靠边隐藏
// ============================================================

void Window_CheckAndAutoHide(HWND hwnd)
{
    // 鼠标离开窗口时触发：窗口在边缘则自动隐藏
    if (s_hideSide != HIDE_NONE) return;

    RECT r;
    GetWindowRect(hwnd, &r);
    int w = r.right - r.left;
    int sw = GetSystemMetrics(SM_CXSCREEN);

    // 靠近左边缘
    if (r.left < EDGE_THRESHOLD)
    {
        s_hideSide = HIDE_LEFT;
        SetWindowPos(hwnd, NULL,
                     HIDE_EDGE - w, r.top,
                     0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
        SetTimer(hwnd, IDT_HIDE, 200, NULL);
    }
    // 靠近右边缘
    else if (r.right > sw - EDGE_THRESHOLD)
    {
        s_hideSide = HIDE_RIGHT;
        SetWindowPos(hwnd, NULL,
                     sw - HIDE_EDGE, r.top,
                     0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
        SetTimer(hwnd, IDT_HIDE, 200, NULL);
    }
}

void Window_OnHideTimer(HWND hwnd)
{
    if (s_hideSide == HIDE_NONE) return;

    POINT pt;
    GetCursorPos(&pt);
    int sw = GetSystemMetrics(SM_CXSCREEN);

    int isNear = 0;
    if (s_hideSide == HIDE_LEFT)
        isNear = (pt.x < HIDE_EDGE + HIDE_MARGIN);
    else // HIDE_RIGHT
        isNear = (pt.x > sw - HIDE_EDGE - HIDE_MARGIN);

    if (isNear)
        Window_PopOut(hwnd);
}

void Window_PopOut(HWND hwnd)
{
    if (s_hideSide == HIDE_NONE) return;

    KillTimer(hwnd, IDT_HIDE);

    RECT r;
    GetWindowRect(hwnd, &r);
    int w = r.right - r.left;

    if (s_hideSide == HIDE_LEFT)
    {
        // 左边缘弹出：回到 X=0
        SetWindowPos(hwnd, NULL, 0, r.top, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }
    else // HIDE_RIGHT
    {
        // 右边缘弹出：回到右对齐
        int sw = GetSystemMetrics(SM_CXSCREEN);
        SetWindowPos(hwnd, NULL, sw - w, r.top, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    s_hideSide = HIDE_NONE;
}

int Window_IsHidden(void)
{
    return (s_hideSide != HIDE_NONE);
}

void Window_ToggleHide(HWND hwnd)
{
    if (s_hideSide != HIDE_NONE)
    {
        Window_PopOut(hwnd);
    }
    else
    {
        // 隐藏到右边缘
        RECT r;
        GetWindowRect(hwnd, &r);
        int w = r.right - r.left;
        int sw = GetSystemMetrics(SM_CXSCREEN);

        s_hideSide = HIDE_RIGHT;
        SetWindowPos(hwnd, NULL, sw - HIDE_EDGE, r.top, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
        SetTimer(hwnd, IDT_HIDE, 200, NULL);
    }
}
