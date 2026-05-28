#include "titlebar.h"

#include <string.h>

/*
 * 命令组位掩码值，与 app.h 中 AppTitlebarCommandGroup 定义保持一致。
 * 本地定义避免 ui/titlebar 反向依赖 app/。
 */
#define TITLEBAR_GROUP_WINDOW_CONTROLS      (1u << 0)
#define TITLEBAR_GROUP_SHELL_MODES          (1u << 1)
#define TITLEBAR_GROUP_BACKGROUND_ACTIONS   (1u << 2)

/* 按钮像素尺寸 */
#define TITLEBAR_BUTTON_WIDTH  46
#define TITLEBAR_TEXT_PADDING  12

/*
 * 边框与标题栏使用的颜色常量。
 */
static const RenderColor TITLEBAR_COLOR_BORDER      = { 180, 180, 180, 255 };
static const RenderColor TITLEBAR_COLOR_BACKGROUND   = { 255, 251, 214, 255 };
static const RenderColor TITLEBAR_COLOR_TEXT         = {  80,  80,  80, 255 };

/* 菜单按钮颜色 — 黄底同色系 */
static const RenderColor MENU_BTN_BG      = { 242, 238, 210, 255 };  /* 比标题栏略深但同色系 */
static const RenderColor MENU_BTN_HOVER   = { 240, 225, 180, 255 };  /* 暖黄 hover */
static const RenderColor MENU_BTN_PRESSED = { 230, 210, 160, 255 };  /* 更深按下次 */

TitlebarLayout Titlebar_CalculateLayout(int window_width,
                                        int window_height,
                                        int titlebar_height,
                                        int frame_visual_thickness,
                                        unsigned int titlebar_command_groups)
{
    TitlebarLayout layout;
    int titlebar_y;
    int current_x;

    (void)titlebar_command_groups;

    memset(&layout, 0, sizeof(layout));

    layout.window_width = window_width;
    layout.window_height = window_height;

    /* 标题栏区域：紧接顶部边框下方 */
    titlebar_y = frame_visual_thickness;
    layout.titlebar_rect.x = 0;
    layout.titlebar_rect.y = titlebar_y;
    layout.titlebar_rect.width = window_width;
    layout.titlebar_rect.height = titlebar_height;

    /* 标题文本区域：左侧留白 */
    layout.title_text_rect.x = TITLEBAR_TEXT_PADDING;
    layout.title_text_rect.y = titlebar_y;
    layout.title_text_rect.width = window_width - TITLEBAR_TEXT_PADDING * 2;
    layout.title_text_rect.height = titlebar_height;

    /* 右上角菜单按钮 */
    current_x = window_width - TITLEBAR_BUTTON_WIDTH;
    Button_Init(&layout.menu_button, current_x, titlebar_y,
                TITLEBAR_BUTTON_WIDTH, titlebar_height);
    layout.menu_button.bg_color = MENU_BTN_BG;
    layout.menu_button.hover_color = MENU_BTN_HOVER;
    layout.menu_button.pressed_color = MENU_BTN_PRESSED;
    layout.menu_button.is_visible = 1;
    layout.menu_button.is_enabled = 1;
    layout.menu_button.label = L"\u2630";  /* Shell-2a_repair-3: ☰ 汉堡图标 */
    layout.menu_button.label_color = (RenderColor){ 140, 120, 80, 255 };  /* 深棕，与黄底搭配 */

    return layout;
}

TitlebarHitResult Titlebar_HitTest(const TitlebarLayout* layout, int x, int y)
{
    if (layout == NULL)
        return TITLEBAR_HIT_NONE;

    /* 优先检查菜单按钮区 */
    if (Button_HitTest(&layout->menu_button, x, y))
        return TITLEBAR_HIT_MENU_BUTTON;

    /* 再检查标题栏拖拽区 */
    if (x >= layout->titlebar_rect.x &&
        x < layout->titlebar_rect.x + layout->titlebar_rect.width &&
        y >= layout->titlebar_rect.y &&
        y < layout->titlebar_rect.y + layout->titlebar_rect.height)
    {
        return TITLEBAR_HIT_DRAG_BAR;
    }

    return TITLEBAR_HIT_NONE;
}

void Titlebar_Draw(RenderContext* ctx, const TitlebarLayout* layout)
{
    RenderRect border_rect;

    if (ctx == NULL || layout == NULL)
        return;

    /* ========== 顶部边框 ========== */
    border_rect.x = 0;
    border_rect.y = 0;
    border_rect.width = layout->window_width;
    border_rect.height = 1;
    Render_FillRect(ctx, border_rect, TITLEBAR_COLOR_BORDER);

    /* ========== 标题栏背景 ========== */
    Render_FillRect(ctx, layout->titlebar_rect, TITLEBAR_COLOR_BACKGROUND);

    /* ========== 标题文本 ========== */
    Render_DrawText(ctx,
                    L"DeskNote",
                    layout->title_text_rect,
                    TITLEBAR_COLOR_TEXT);

    /* ========== 菜单按钮 ========== */
    Button_Draw(ctx, &layout->menu_button);

    /* ========== 左边框 ========== */
    border_rect.x = 0;
    border_rect.y = 0;
    border_rect.width = 1;
    border_rect.height = layout->window_height;
    Render_FillRect(ctx, border_rect, TITLEBAR_COLOR_BORDER);

    /* ========== 右边框 ========== */
    border_rect.x = layout->window_width - 1;
    border_rect.y = 0;
    border_rect.width = 1;
    border_rect.height = layout->window_height;
    Render_FillRect(ctx, border_rect, TITLEBAR_COLOR_BORDER);

    /* ========== 下边框 ========== */
    border_rect.x = 0;
    border_rect.y = layout->window_height - 1;
    border_rect.width = layout->window_width;
    border_rect.height = 1;
    Render_FillRect(ctx, border_rect, TITLEBAR_COLOR_BORDER);
}
