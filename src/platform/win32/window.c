#include "../../app/app.h"
#include <windows.h>
#include "window.h"

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
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

    case WM_SIZE:
        (void)App_OnResize((unsigned int)LOWORD(lParam), (unsigned int)HIWORD(lParam));
        return 0;

    case WM_DESTROY:
        App_Shutdown();
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
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(
        0,
        L"DeskNoteWindow",
        L"DeskNote",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, instance, NULL);

    if (!hwnd)
        return 1;

    if (App_Init(hwnd) != 0)
    {
        DestroyWindow(hwnd);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

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
