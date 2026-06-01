#ifndef DESKNOTE_TITLEBAR_H
#define DESKNOTE_TITLEBAR_H

#include "../render/render.h"
#include "../ui/button.h"

/*
 * 标题栏布局计算结果。
 *
 * menu_button 是标题栏上唯一的按钮，点击弹出上下文菜单。
 * (close_button + minimize_button 已在 Shell-2a 中替换为 menu_button)
 */
typedef struct {
    RenderRect titlebar_rect;          /* 标题栏背景区域（整行宽度，titlebar_height 高度）*/
    RenderRect title_text_rect;        /* 标题文本显示区域 */
    Button menu_button;                /* 菜单按钮（替换旧 close_button + minimize_button）*/
    Button status_indicator;           /* Shell-5d: 状态指示灯（仅视觉，不响应点击）*/
    int window_width;                  /* 窗口客户区宽度（供边框绘制使用）*/
    int window_height;                 /* 窗口客户区高度（供边框绘制使用）*/
} TitlebarLayout;

/*
 * 标题栏命中区域枚举。
 * 用于 App_OnLeftButtonDown 中判断点击落在哪块区域。
 */
typedef enum {
    TITLEBAR_HIT_NONE = 0,
    TITLEBAR_HIT_MENU_BUTTON,
    TITLEBAR_HIT_DRAG_BAR
} TitlebarHitResult;

/*
 * 标题栏命中测试。
 * 先检查菜单按钮区，再检查拖拽区。
 */
TitlebarHitResult Titlebar_HitTest(const TitlebarLayout* layout, int x, int y);

/*
 * 根据窗口参数计算标题栏布局。
 */
TitlebarLayout Titlebar_CalculateLayout(int window_width,
                                        int window_height,
                                        int titlebar_height,
                                        int frame_visual_thickness,
                                        unsigned int titlebar_command_groups);

/*
 * 绘制标题栏（背景、标题文本、菜单按钮、状态指示灯）和窗口边框。
 */
void Titlebar_Draw(RenderContext* ctx, const TitlebarLayout* layout);

/*
 * Shell-5d: 根据 resident_mode 刷新状态指示灯的外观。
 * 在 App_OnPaint 中、Titlebar_CalculateLayout 之后、Titlebar_Draw 之前调用。
 */
void Titlebar_UpdateStatus(Button* indicator, int mode);

#endif /* DESKNOTE_TITLEBAR_H */
