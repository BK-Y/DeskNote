#include "button.h"
#include <string.h>

void Button_Init(Button* btn, int x, int y, int width, int height)
{
    if (btn == NULL)
        return;
    memset(btn, 0, sizeof(*btn));
    btn->rect.x = x;
    btn->rect.y = y;
    btn->rect.width = width;
    btn->rect.height = height;
    btn->is_visible = 1;
    btn->is_enabled = 1;
    btn->state = BUTTON_STATE_NORMAL;
}

int Button_HitTest(const Button* btn, int mx, int my)
{
    if (btn == NULL)
        return 0;
    return (mx >= btn->rect.x &&
            mx < btn->rect.x + btn->rect.width &&
            my >= btn->rect.y &&
            my < btn->rect.y + btn->rect.height) ? 1 : 0;
}

void Button_UpdateState(Button* btn, int mx, int my, int mouse_down)
{
    int hovered;
    if (btn == NULL)
        return;
    if (!btn->is_visible || !btn->is_enabled)
        return;
    hovered = Button_HitTest(btn, mx, my);
    if (hovered && mouse_down)
        btn->state = BUTTON_STATE_PRESSED;
    else if (hovered && !mouse_down)
        btn->state = BUTTON_STATE_HOVER;
    else
        btn->state = BUTTON_STATE_NORMAL;
}

void Button_Draw(RenderContext* ctx, const Button* btn)
{
    RenderColor fill_color;
    RenderColor text_color;

    if (ctx == NULL || btn == NULL || !btn->is_visible)
        return;

    switch (btn->state)
    {
    case BUTTON_STATE_PRESSED:
        fill_color = btn->pressed_color;
        break;
    case BUTTON_STATE_HOVER:
        fill_color = btn->hover_color;
        break;
    default:
        fill_color = btn->bg_color;
        break;
    }

    Render_FillRect(ctx, btn->rect, fill_color);

    if (btn->label != NULL)
    {
        text_color = btn->label_color;
        if (text_color.r == 0 && text_color.g == 0 && text_color.b == 0)
            text_color = (RenderColor){ 40, 40, 40, 255 };
        Render_DrawTextCentered(ctx, btn->label, btn->rect, text_color);
    }
}
