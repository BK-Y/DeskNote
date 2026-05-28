#include "nonclient.h"
#include <windows.h>
#include <windowsx.h>

static int g_custom_chrome_enabled = 0;
static int g_titlebar_height = 32;
static int g_frame_visual_thickness = 1;
static int g_frame_resize_thickness = 6;

static int Platform_Nonclient_ClampNonNegative(int value)
{
    return value < 0 ? 0 : value;
}

int Platform_Nonclient_Init(HWND hwnd)
{
    (void)hwnd;
    /* Prefer system caption metric as a sensible default */
    int sys = GetSystemMetrics(SM_CYCAPTION);
    if (sys > 0)
        g_titlebar_height = sys;
    return 0;
}

int Platform_Nonclient_SetMetrics(HWND hwnd,
                                  int enabled,
                                  int titlebar_height,
                                  int frame_visual_thickness,
                                  int frame_resize_thickness)
{
    (void)hwnd;

    g_custom_chrome_enabled = enabled ? 1 : 0;
    if (titlebar_height > 0)
        g_titlebar_height = titlebar_height;
    g_frame_visual_thickness = Platform_Nonclient_ClampNonNegative(frame_visual_thickness);
    g_frame_resize_thickness = Platform_Nonclient_ClampNonNegative(frame_resize_thickness);
    return 0;
}

int Platform_Nonclient_IsEnabled(void)
{
    return g_custom_chrome_enabled;
}

int Platform_Nonclient_ComputeContentRect(HWND hwnd, RECT* out_rect)
{
    int frame_inset;
    int top_inset;

    if (hwnd == NULL || out_rect == NULL)
        return 1;
    if (!GetClientRect(hwnd, out_rect))
        return 1;
    if (!g_custom_chrome_enabled)
        return 0;

    frame_inset = g_frame_visual_thickness;
    top_inset = g_titlebar_height + g_frame_visual_thickness;

    out_rect->left += frame_inset;
    out_rect->top += top_inset;
    out_rect->right -= frame_inset;
    out_rect->bottom -= frame_inset;

    if (out_rect->right < out_rect->left)
        out_rect->right = out_rect->left;
    if (out_rect->bottom < out_rect->top)
        out_rect->bottom = out_rect->top;

    return 0;
}

LRESULT Platform_Nonclient_HandleNCHitTest(HWND hwnd, LPARAM lParam)
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    RECT wr;
    if (!GetWindowRect(hwnd, &wr))
        return HTCLIENT;

    int x = pt.x - wr.left;
    int y = pt.y - wr.top;
    int w = wr.right - wr.left;

    /*
     * Titlebar area: y in [0, g_titlebar_height) after frame_visual_thickness offset.
     * Menu button is at the right edge (46px wide) — return HTCLIENT to
     * let app layer handle the click. Everything else is HTCAPTION for dragging.
     */
    if (y >= g_frame_visual_thickness &&
        y < g_frame_visual_thickness + g_titlebar_height)
    {
        /* Menu button region: rightmost 46px */
        if (x >= w - 46 && x < w)
            return HTCLIENT;
        return HTCAPTION;
    }

    return HTCLIENT;
}

void Platform_Nonclient_Shutdown(HWND hwnd)
{
    (void)hwnd;
    /* No resources to free in this minimal implementation. */
}
