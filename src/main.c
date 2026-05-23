// DeskNote - 轻量桌面便签
// 阶段 1：第一个窗口 - 注册 → 创建 → 消息循环
//
// 窗口程序四步曲：
//   ① RegisterClassExW  注册窗口类
//   ② CreateWindowExW   创建窗口
//   ③ ShowWindow        显示窗口
//   ④ while(GetMessage) 消息循环（直到收到 WM_QUIT 退出）

#define _WIN32_WINNT 0x0600
#ifndef UNICODE
#define UNICODE       // 告诉 Windows：我要用宽字符版本 API
#include <winuser.h>
#endif
#ifndef _UNICODE
#define _UNICODE      // C 运行库也要宽字符版本
#endif

#include <windows.h>  // Win32 API 全部定义在这里
#include "ui/titlebar.h"
#include "ui/window.h"
#include "storage.h"
#include "notefile.h"
#include "md_parser.h"
#define TITLEBAR_HEIGHT 30

// 右键菜单命令 ID
#define IDM_SETTINGS    1001
#define IDM_VIEW_ALL    1002
#define IDM_ABOUT       1003



// ============================================================
// WndProc - 窗口过程（回调函数）
//
// Windows 在发生事件时自动调用这个函数，
// 我们不需要主动调用它，只需在这里告诉 Windows"每种事件怎么处理"。
//
// 参数：
//   hwnd - 收到消息的窗口句柄（标识具体哪个窗口）
//   msg  - 消息类型（WM_DESTROY, WM_PAINT, WM_CLOSE...）
//   wp   - 附加参数1（内容因 msg 而异）
//   lp   - 附加参数2（内容因 msg 而异）
//
// 返回值：
//   已处理的消息 → return 0
//   未处理的消息 → return DefWindowProc(...) 交给系统默认处理
// ============================================================
LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:
        // 窗口正在销毁，发出退出消息给消息循环
        // PostQuitMessage(0) 会让 GetMessage 返回 FALSE，结束循环
        PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN:
        {
            int x=(int)(short)LOWORD(lp);
            int y=(int)(short)HIWORD(lp);
            int hit = Titlebar_HitTest(hwnd, x, y);

            if (hit == -1) SendMessage(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
            else if (hit == TITLEBAR_BTN_CLOSE) PostMessageW(hwnd,WM_CLOSE,0,0);
            else if (hit == TITLEBAR_BTN_PIN) Titlebar_TogglePin(hwnd);
            else if (hit == TITLEBAR_BTN_HIDE) Window_ToggleHide(hwnd);

            return 0;
        }
        break;
    case WM_ACTIVATE:
        // 失去焦点时自动保存
        break;

    case WM_CLOSE:
        // 保存并更新 last_file 配置
        // NoteFile_Close(NULL);
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd,&ps);
            Titlebar_Draw(hwnd,hdc);
            EndPaint(hwnd,&ps);
            return 0;
        }
    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);  // 强制重绘整个窗口
        return 0;
    case WM_TIMER:
        if (wp == 2001) Window_OnHideTimer(hwnd);
        return 0;
    case WM_NCHITTEST:
        {
            POINT pt = { LOWORD(lp),HIWORD(lp)};
            ScreenToClient(hwnd,&pt);
            int hit = Window_ResizeHitTest(hwnd,pt.x,pt.y);
            if (hit) return hit;
            return DefWindowProc(hwnd,msg,wp,lp);
        }
    case WM_MOUSEMOVE:
        int x = LOWORD(lp);
        int y = HIWORD(lp);
        Titlebar_OnMouseMove(hwnd,x,y);
        TRACKMOUSEEVENT tme = { sizeof(tme),TME_LEAVE,hwnd,0};
        TrackMouseEvent(&tme);
        break;

    case WM_MOUSELEAVE:
        Titlebar_OnMouseLeave(hwnd);
        Window_CheckAndAutoHide(hwnd);
        break;

    case WM_RBUTTONDOWN:
    {
        int y = HIWORD(lp);
        if (y >= 0 && y < TITLEBAR_HEIGHT)
        {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, IDM_SETTINGS, L"设置");
            AppendMenuW(menu, MF_STRING, IDM_VIEW_ALL, L"查看全部Note");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, IDM_ABOUT, L"关于 DeskNote");

            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(menu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
            return 0;
        }
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wp);
        if (id == IDM_ABOUT)
        {
            MessageBoxW(hwnd,
                L"DeskNote v0.1.0\n\n轻量桌面便签\n\n开源工具链：\nMinGW-w64 GCC + CMake\n\nWin32 API + md4c",
                                L"关于 DeskNote",
                MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (id == IDM_SETTINGS)
        {
            MessageBoxW(hwnd, L"设置对话框将在后续版本实现。",
                L"设置", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (id == IDM_VIEW_ALL)
        {
            MessageBoxW(hwnd, L"查看全部Note功能将在后续版本实现。",
                L"查看全部Note", MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        break;
    }



    }
    // 其余消息（关闭、重绘、鼠标点击...）交给系统默认处理
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ============================================================
// WinMain - Windows GUI 程序的入口点
//
// 为什么不是 main()？
//   - main()     → 控制台程序，启动时有黑窗口
//   - WinMain()  → GUI 程序，无控制台，直接显示窗口
//   - CMakeLists.txt 中 add_executable(desknote WIN32 ...)
//     告诉链接器入口是 WinMain 而不是 main
//
// 参数：
//   hInstance     - 当前程序的"身份证"，标识这个 exe
//   hPrevInstance - 16位 Windows 遗留物，永远 NULL
//   lpCmdLine     - 命令行参数（字符串指针）
//   nCmdShow      - 窗口显示方式（最大化/最小化/正常）
// ============================================================
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    // ============================================================
    // ① 注册窗口类
    //
    // WNDCLASSEXW 就是"窗口设计图纸"：
    //   告诉 Windows "我这种窗口长这样，消息发到 WndProc"
    //
    // 注册成功后返回一个 ATOM（非零整数），
    // 注册失败返回 0，用 !RegisterClassExW 判断
    // ============================================================
    WNDCLASSEXW wc = { sizeof(wc) };      // cbSize 必须填结构体大小
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; // 窗口大小变化时重绘
    wc.lpfnWndProc   = WndProc;            // ← 关键！消息处理函数的地址
    wc.hInstance     = hInstance;          // 这个窗口类属于当前程序
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW); // 标准箭头光标
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);   // 白色背景
    wc.lpszClassName = L"DeskNote";        // 窗口类名称（CreateWindow 时用这个名）
    wc.hIcon         = LoadIconW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(1));
    wc.hIconSm       = NULL;


    if (!RegisterClassExW(&wc))
        return 1;  // 注册失败 → 退出

    // ============================================================
    // ② 创建窗口实例
    //
    // 参数含义：
    //   0              - 扩展样式（后面会用 WS_EX_TOOLWINDOW）
    //   L"DeskNote"    - 窗口类名（必须和注册时一致）
    //   L"DeskNote"    - 窗口标题（后面会改成便签文件名）
    //   WS_OVERLAPPEDWINDOW - 标准窗口（有标题栏、可改变大小）
    //   CW_USEDEFAULT  - 位置让系统自动选
    //   800, 600       - 宽度, 高度
    //   NULL           - 父窗口（无）
    //   NULL           - 菜单（无）
    //   hInstance      - 程序实例
    //   NULL           - 额外参数（后面会用来传递便签数据）
    //
    // 返回 HWND（窗口句柄），失败返回 NULL
    // ============================================================
    HWND hwnd = CreateWindowExW(
        0,
        L"DeskNote",
        L"DeskNote",
        WS_POPUP,
        //WS_POPUP|WS_THICKFRAME,
        GetSystemMetrics(SM_CXSCREEN)-240, 0,
        240, 360,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return 1;  // 创建失败 → 退出

    // ============================================================
    // ③ 显示窗口
    //
    // nCmdShow 是 WinMain 的参数，通常是 SW_SHOWNORMAL（正常显示）
    // Windows 也可能传 SW_SHOWMINIMIZED（最小化）等
    // ============================================================
    ShowWindow(hwnd, nCmdShow);

    // 初始化存储目录
    Storage_Init();

    // ============================================================
    // ④ 消息循环（Message Loop）
    //
    // 这就是程序的"心脏"：
    //   GetMessage(&msg, NULL, 0, 0)
    //     从 Windows 拿一条消息，如果没有消息就挂起（不占 CPU）
    //     收到 WM_QUIT 时返回 FALSE，循环结束
    //
    //   TranslateMessage(&msg)
    //     把键盘消息翻译成字符消息（比如 WM_KEYDOWN → WM_CHAR）
    //
    //   DispatchMessage(&msg)
    //     把消息交给 WndProc 处理
    //
    // 循环结束后 msg.wParam 就是 PostQuitMessage(0) 传的退出码
    // ============================================================
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
