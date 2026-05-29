#include "../../app/app.h"
#include <windows.h>
#include <windowsx.h>
#include "window.h"
#include "nonclient.h"

#include <winerror.h>

enum {
    WINDOW_AUTOSAVE_TIMER_ID = 1,
    WINDOW_AUTOSAVE_TIMER_INTERVAL_MS = 250
};

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCHITTEST:
        if (Platform_Nonclient_IsEnabled())
            return Platform_Nonclient_HandleNCHitTest(hwnd, lParam);
        break;

    case WM_CHAR:
        App_OnChar((wchar_t)wParam);
        return 0;

    case WM_SETFOCUS:
        App_OnFocusGained();
        return 0;

    case WM_KILLFOCUS:
        App_OnFocusLost();
        return 0;

    case WM_IME_STARTCOMPOSITION:
        App_OnImeStartComposition();
        break;

    case WM_KEYDOWN:
        App_OnKeyDown((unsigned int)wParam);
        return 0;

    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        App_OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }

    case WM_MOUSELEAVE:
        App_OnMouseLeave();
        return 0;

    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        App_OnLeftButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL:
        App_OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_TIMER:
        if (wParam == WINDOW_AUTOSAVE_TIMER_ID)
        {
            /* Shell-3a_2: 在处理自动保存前，先收束待处理的托盘命令 */
            AppShellCommand cmd;
            if (App_TakePendingShellCommand(&cmd) == 0)
            {
                switch (cmd)
                {
                    case APP_SHELL_COMMAND_RESTORE_FROM_TRAY:
                        App_RestoreFromTray(hwnd);
                        break;
                    case APP_SHELL_COMMAND_SHOW_TRAY_MENU:
                        Platform_ShowTrayMenu(hwnd);
                        break;
                    default:
                        break;
                }
            }

            App_OnTick();
            return 0;
        }
        break;

    case WM_SIZE:
        (void)App_OnResize((unsigned int)LOWORD(lParam), (unsigned int)HIWORD(lParam));
        return 0;

    /* Shell-3a_2: 托盘消息 — 只做翻译，不做执行 */
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP)
            App_SubmitShellCommand(APP_SHELL_COMMAND_RESTORE_FROM_TRAY);
        else if (lParam == WM_RBUTTONUP)
            App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_TRAY_MENU);
        return 0;

    /* Shell-3a_2: 托盘菜单命令 */
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case 1001: /* 托盘菜单：显示窗口 */
                App_SubmitShellCommand(APP_SHELL_COMMAND_RESTORE_FROM_TRAY);
                break;
            case 1002: /* 托盘菜单：退出 */
                DestroyWindow(hwnd);
                break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, WINDOW_AUTOSAVE_TIMER_ID);
        App_DestroyTrayIcon(hwnd);
        App_Shutdown();
        Platform_Nonclient_Shutdown(hwnd);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint(hwnd, &ps);
        App_OnPaint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int Window_Run(void)
{
    HINSTANCE instance = GetModuleHandleW(NULL);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"DeskNoteWindow";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;  /* 背景由 Render_Clear 完全掌控，避免 BeginPaint 预填白色 */

    if (!RegisterClassW(&wc))
        return 1;

    /* Shell-2b_1a: 计算初始位置（屏幕右上角）*/
    int window_x = CW_USEDEFAULT;
    int window_y = CW_USEDEFAULT;
    {
        RECT work_area;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        {
            window_x = (int)(work_area.right - 240);
            window_y = (int)work_area.top;
        }
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"DeskNoteWindow",
        L"DeskNote",
        WS_OVERLAPPEDWINDOW,
        window_x, window_y,
        240, 388,       /* Shell-2b_1: 默认便签尺寸 */
        NULL, NULL, instance, NULL);

    if (!hwnd)
        return 1;

    /* Initialize minimal non-client handling for frameless support */
    (void)Platform_Nonclient_Init(hwnd);

    if (App_Init(hwnd) != 0)
    {
        DestroyWindow(hwnd);
        return 1;
    }

    /* Shell-3a_1: 初始化托盘图标（启动即有，无需条件） */
    (void)App_InitTrayIcon(hwnd);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    if (SetTimer(hwnd, WINDOW_AUTOSAVE_TIMER_ID, WINDOW_AUTOSAVE_TIMER_INTERVAL_MS, NULL) == 0)
    {
        DestroyWindow(hwnd);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return App_Run();
}

int Platform_SetFrameless(HWND hwnd, int enable)
{
    if (hwnd == NULL)
        return 1;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (enable)
    {
        style &= ~((LONG_PTR)WS_OVERLAPPEDWINDOW);
        style |= (LONG_PTR)WS_POPUP;
    }
    else
    {
        style &= ~((LONG_PTR)WS_POPUP);
        style |= (LONG_PTR)WS_OVERLAPPEDWINDOW;
    }

    if (SetWindowLongPtrW(hwnd, GWL_STYLE, style) == 0 && GetLastError() != 0)
        return 1;

    /* Force the window to apply new frame styles */
    if (!SetWindowPos(hwnd,
                      NULL,
                      0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED))
    {
        return 1;
    }

    return 0;
}

int Platform_ComputeContentRect(HWND hwnd, RECT* out_rect)
{
    return Platform_Nonclient_ComputeContentRect(hwnd, out_rect);
}

void Platform_BeginWindowDrag(HWND hwnd)
{
    if (hwnd == NULL)
        return;

    ReleaseCapture();
    SendMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
}

/* Shell-3a_2: 弹出托盘右键菜单 */
void Platform_ShowTrayMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1001, L"显示窗口");
    AppendMenuW(hMenu, MF_STRING, 1002, L"退出");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

int Platform_RebuildClientArea(HWND hwnd)
{
    if (hwnd == NULL)
        return 1;

    /* For now the rebuild simply notifies app layer to re-query content rect. */
    (void)hwnd;
    App_RequestClientRebuild();
    return 0;
}
