/* Shell-5a: AppBar 注册与贴边模型 */
#include "appbar.h"
#include "../../storage/state_store.h"

#include <wchar.h>      /* diag: swprintf */
#include <debugapi.h>   /* diag: OutputDebugStringW */

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
    s_appbar.data.uCallbackMessage = (UINT)(WM_APP + 2);

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

    /* Workaround: 先把 AppBar 劈到一条不冲突的边（底部），再移除，
     * 避免 ABM_REMOVE 偶尔不释放原边保留区 */
    s_appbar.data.uEdge = (UINT)APP_DOCK_BOTTOM;
    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    SHAppBarMessage(ABM_REMOVE, &s_appbar.data);
    s_appbar.registered = 0;

    /* 强制 Windows 刷新工作区 */
    SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, 0);
    return 0;
}

/* Shell-5b 反馈循环防护：标记当前工作区变化是否由我们自己的 ABM_SETPOS 触发 */
static int s_own_wa_change = 0;

void AppBar_MarkOwnWorkareaChange(void)
{
    s_own_wa_change = 1;
}

int AppBar_ConsumeOwnWorkareaChange(void)
{
    if (s_own_wa_change)
    {
        s_own_wa_change = 0;
        return 1;
    }
    return 0;
}

int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);

    /* diag: 工作区宽度 */
    {
        wchar_t buf[128];
        swprintf(buf, 128, L"[s5a-diag] workarea_w=%d thickness=%d\n",
                 rc.right - rc.left, thickness);
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

    SHAppBarMessage(ABM_QUERYPOS, &s_appbar.data);
    AppBar_MarkOwnWorkareaChange();  /* 标记：接下来的变化是我们自己引起的 */
    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    /* diag: SETPOS 后的 AppBar 矩形 */
    {
        wchar_t buf[128];
        swprintf(buf, 128, L"[s5a-diag] setpos_rc=(%d,%d,%d,%d) w=%d\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right, s_appbar.data.rc.bottom,
                 s_appbar.data.rc.right - s_appbar.data.rc.left);
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

int AppBar_ReRegister(HWND hwnd)
{
    AppDockEdge edge;
    int thickness;

    if (!s_appbar.registered)
        return 0;

    edge = (AppDockEdge)s_appbar.data.uEdge;
    thickness = s_appbar.data.rc.right - s_appbar.data.rc.left;

    s_appbar.registered = 0;
    if (AppBar_Register(hwnd) != 0)
        return 1;

    return AppBar_SetPosition(hwnd, edge, thickness);
}

int AppBar_ReadDockConfig(AppDockEdge* out_edge, int* out_thickness)
{
    StateData state;
    StateStore_Load(&state);
    if (out_edge)
        *out_edge = (AppDockEdge)state.dock_edge;
    if (out_thickness)
        *out_thickness = state.dock_thickness;
    return 0;
}

AppDockEdge AppBar_GetEdge(void)
{
    return (AppDockEdge)s_appbar.data.uEdge;
}

int AppBar_GetThickness(void)
{
    return s_appbar.data.rc.right - s_appbar.data.rc.left;
}
