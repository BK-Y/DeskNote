/* Shell-5a: AppBar 注册与贴边模型 */
#include "appbar.h"
#include "../../storage/state_store.h"

#include <wchar.h>       /* phase-10-shell-5a-repair-3: swprintf */
#include <debugapi.h>    /* phase-10-shell-5a-repair-3: OutputDebugStringW */

typedef struct {
    APPBARDATA data;
    int registered;
} AppBarState;

static AppBarState s_appbar = {0};

int AppBar_Register(HWND hwnd)
{
    if (s_appbar.registered)
        return 0;

    memset(&s_appbar.data, 0, sizeof(s_appbar.data));
    s_appbar.data.cbSize = sizeof(s_appbar.data);
    s_appbar.data.hWnd = hwnd;
    s_appbar.data.uCallbackMessage = (UINT)(WM_APP + 2); /* AppBar 通知消息 */

    if (!SHAppBarMessage(ABM_NEW, &s_appbar.data))
        return 1;

    s_appbar.registered = 1;
    return 0;
}

int AppBar_Unregister(HWND hwnd)
{
    (void)hwnd;
    if (!s_appbar.registered)
        return 0;

    SHAppBarMessage(ABM_REMOVE, &s_appbar.data);
    s_appbar.registered = 0;
    return 0;
}

int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 1 — 原始工作区 ---- */
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[Shell-5a-repair-3] workarea: L=%d T=%d R=%d B=%d\r\n",
                 rc.left, rc.top, rc.right, rc.bottom);
        OutputDebugStringW(buf);
    }

    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;

    switch (edge)
    {
    case APP_DOCK_LEFT:
        s_appbar.data.rc.right = rc.left + thickness;
        break;
    case APP_DOCK_RIGHT:
        s_appbar.data.rc.left = rc.right - thickness;
        break;
    case APP_DOCK_TOP:
        s_appbar.data.rc.bottom = rc.top + thickness;
        break;
    case APP_DOCK_BOTTOM:
        s_appbar.data.rc.top = rc.bottom - thickness;
        break;
    }

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 2 — 我们自己设置的 rc ---- */
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[Shell-5a-repair-3] before ABM_QUERYPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right, s_appbar.data.rc.bottom,
                 s_appbar.data.rc.right - s_appbar.data.rc.left,
                 s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    SHAppBarMessage(ABM_QUERYPOS, &s_appbar.data);

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 3 — QUERYPOS 之后 ---- */
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[Shell-5a-repair-3] after ABM_QUERYPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right, s_appbar.data.rc.bottom,
                 s_appbar.data.rc.right - s_appbar.data.rc.left,
                 s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 4 — SETPOS 之后 ---- */
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[Shell-5a-repair-3] after ABM_SETPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right, s_appbar.data.rc.bottom,
                 s_appbar.data.rc.right - s_appbar.data.rc.left,
                 s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 5 — 最终 MoveWindow 参数 ---- */
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[Shell-5a-repair-3] MoveWindow: x=%d y=%d w=%d h=%d\r\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right - s_appbar.data.rc.left,
                 s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    MoveWindow(hwnd,
               s_appbar.data.rc.left,
               s_appbar.data.rc.top,
               s_appbar.data.rc.right - s_appbar.data.rc.left,
               s_appbar.data.rc.bottom - s_appbar.data.rc.top,
               TRUE);

    return 0;
}

int AppBar_IsRegistered(HWND hwnd)
{
    (void)hwnd;
    return s_appbar.registered;
}
