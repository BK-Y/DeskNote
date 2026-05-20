// titlebar.c - 使用 Widget 构件库的标题栏
#include "titlebar.h"
#include "../../lib/widget.h"

// ============================================================
// 按钮布局常量（编译期运算）
// ============================================================

#define BTN_W   22
#define BTN_GAP 2
#define BTN_TOP 3
#define BTN_H   (TITLEBAR_HEIGHT - BTN_TOP - 2)

// 从右向左的偏移量（编译期常量）
#define OFF_CLOSE  0
#define OFF_PIN    (OFF_CLOSE + BTN_W + BTN_GAP)   // = 24
#define OFF_HIDE   (OFF_PIN   + BTN_W + BTN_GAP)   // = 48
#define BTNS_WIDTH (OFF_HIDE + BTN_W)               // = 70

// ============================================================
// 全局状态
// ============================================================

static Widget g_btns[3];   // 三个按钮：关闭、置顶、隐藏
static int    g_inited = 0;
static int    s_pinActive = 0;  // 置顶状态

// ============================================================
// 标题栏按钮的自定义绘制
// 标题栏背景是黄色的，不能用 widget 默认的灰色按钮
// ============================================================

static void DrawTitlebarBtn(HDC hdc, Widget* w)
{
    SetBkMode(hdc, TRANSPARENT);

    if (w->id == TITLEBAR_BTN_CLOSE && (w->state & WIDGET_HOVERED))
    {
        HBRUSH red = CreateSolidBrush(RGB(232, 17, 35));
        FillRect(hdc, &w->rect, red);
        DeleteObject(red);
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    else if (w->id == TITLEBAR_BTN_PIN && (w->state & WIDGET_ACTIVE))
    {
        HBRUSH blue = CreateSolidBrush(RGB(0, 120, 215));
        FillRect(hdc, &w->rect, blue);
        DeleteObject(blue);
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    else if (w->state & WIDGET_HOVERED)
    {
        HBRUSH gray = CreateSolidBrush(RGB(200, 200, 200));
        FillRect(hdc, &w->rect, gray);
        DeleteObject(gray);
        SetTextColor(hdc, RGB(50, 50, 50));
    }
    else
    {
        SetTextColor(hdc, RGB(120, 120, 120));
    }

    DrawTextW(hdc, w->text, -1, &w->rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// 初始化按钮（只执行一次）
// ============================================================

static void InitButtons(HWND hwnd)
{
    if (g_inited) return;
    g_inited = 1;

    RECT rect;
    GetClientRect(hwnd, &rect);
    int right = rect.right;

    // 从右向左排列：关闭、置顶、隐藏
    int x = right - OFF_CLOSE - BTN_W;
    g_btns[0] = (Widget){ TITLEBAR_BTN_CLOSE, 0,
        { x, BTN_TOP, x + BTN_W, BTN_TOP + BTN_H },
        0, L"\u00D7", NULL, DrawTitlebarBtn, NULL };

    x = right - OFF_PIN - BTN_W;
    g_btns[1] = (Widget){ TITLEBAR_BTN_PIN, 0,
        { x, BTN_TOP, x + BTN_W, BTN_TOP + BTN_H },
        0, L"\U0001F4CC", NULL, DrawTitlebarBtn, NULL };

    x = right - OFF_HIDE - BTN_W;
    g_btns[2] = (Widget){ TITLEBAR_BTN_HIDE, 0,
        { x, BTN_TOP, x + BTN_W, BTN_TOP + BTN_H },
        0, L"\U0001F5D5", NULL, DrawTitlebarBtn, NULL };
}

// ============================================================
// 按钮重绘（只刷新标题栏区域）
// ============================================================

static void RedrawTitlebar(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    rect.bottom = TITLEBAR_HEIGHT;
    InvalidateRect(hwnd, &rect, TRUE);
}

// ============================================================
// 公共接口
// ============================================================

void Titlebar_Draw(HWND hwnd, HDC hdc)
{
    InitButtons(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);

    // 画黄色标题栏背景
    HBRUSH titleBrush = CreateSolidBrush(RGB(255, 220, 80));
    RECT titleRect = { rect.left, rect.top, rect.right, TITLEBAR_HEIGHT };
    FillRect(hdc, &titleRect, titleBrush);
    DeleteObject(titleBrush);

    // 标题文字（预留按钮空间）
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(50, 50, 50));
    RECT textRect = { 5, 3, rect.right - BTNS_WIDTH - 5, TITLEBAR_HEIGHT };
    DrawTextW(hdc, L"DeskNote", -1, &textRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // 依次绘制按钮（widget 统一绘制）
    Widget_Draw(hdc, &g_btns[0]);
    Widget_Draw(hdc, &g_btns[1]);
    Widget_Draw(hdc, &g_btns[2]);
}

int Titlebar_HitTest(HWND hwnd, int x, int y)
{
    if (y < 0 || y >= TITLEBAR_HEIGHT)
        return 0;  // 内容区

    // 先检测是否点中了某个按钮
    for (int i = 0; i < 3; i++)
    {
        int id = Widget_HitTest(&g_btns[i], x, y);
        if (id)
            return id;
    }

    return -1;  // 标题栏普通区域 → 拖拽
}

void Titlebar_OnMouseMove(HWND hwnd, int x, int y)
{
    InitButtons(hwnd);
    Widget_UpdateHover(g_btns, 3, x, y);

    // 更新置顶按钮的激活状态
    if (s_pinActive)
        g_btns[1].state |= WIDGET_ACTIVE;
    else
        g_btns[1].state &= ~WIDGET_ACTIVE;

    // 有悬停变化时重绘标题栏
    int needRedraw = 0;
    for (int i = 0; i < 3; i++)
    {
        if (g_btns[i].state & WIDGET_HOVERED)
        {
            needRedraw = 1;
            break;
        }
    }
    if (needRedraw)
        RedrawTitlebar(hwnd);
}

void Titlebar_OnMouseLeave(HWND hwnd)
{
    // 清除所有按钮的悬停状态
    for (int i = 0; i < 3; i++)
        g_btns[i].state &= ~WIDGET_HOVERED;

    RedrawTitlebar(hwnd);
}

void Titlebar_TogglePin(HWND hwnd)
{
    s_pinActive = !s_pinActive;

    SetWindowPos(hwnd,
        s_pinActive ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

    // 更新按钮状态并重绘
    if (s_pinActive)
        g_btns[1].state |= WIDGET_ACTIVE;
    else
        g_btns[1].state &= ~WIDGET_ACTIVE;

    RedrawTitlebar(hwnd);
}
