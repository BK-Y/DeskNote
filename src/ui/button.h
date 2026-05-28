#ifndef DESKNOTE_BUTTON_H
#define DESKNOTE_BUTTON_H

#include "../render/render.h"

typedef enum {
    BUTTON_STATE_NORMAL = 0,
    BUTTON_STATE_HOVER,
    BUTTON_STATE_PRESSED
} ButtonState;

typedef struct {
    RenderRect rect;
    ButtonState state;
    int is_visible;
    int is_enabled;
    RenderColor bg_color;
    RenderColor hover_color;
    RenderColor pressed_color;
    RenderColor label_color;       /* 标签文字颜色。全零时 Button_Draw 默认降级为 (40,40,40) */
    const wchar_t* label;
} Button;

void Button_Init(Button* btn, int x, int y, int width, int height);
int Button_HitTest(const Button* btn, int mx, int my);
void Button_UpdateState(Button* btn, int mx, int my, int mouse_down);
void Button_Draw(RenderContext* ctx, const Button* btn);

#endif /* DESKNOTE_BUTTON_H */
