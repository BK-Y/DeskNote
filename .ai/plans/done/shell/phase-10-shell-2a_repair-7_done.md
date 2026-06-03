# Shell-2a_repair-7 — 验证菜单按钮 hover 状态驱动通道是否正常（后续）

## 阶段摘要

用极明显颜色差异验证 Button 的 state 驱动 → Button_Draw 选色 → 渲染的完整链路是否正常。

## 阶段目标

1. 将菜单按钮的 hover_color 临时改为亮红色 (255,80,80)，与 bg_color (242,238,210) 拉开极大差异
2. 编译运行，观察鼠标移上 ☰ 按钮时是否变为亮红色
3. 根据结果判断状态驱动通道是否正常

## 为什么要做这个

当前无法判断 hover 不生效的根因是"色差太小肉眼不可辨"还是"状态驱动链路本身未接通"。必须先用极明显差异消除"看不清"这个变量。

## 阶段产出

1. 明确状态驱动链路正常——颜色可正确按 state 切换
2. 发现问题：App_OnMouseMove 中 x 坐标范围写成左上角 [frame_thickness, frame_thickness+46)，实际按钮在右上角 [width-46, width)
3. 修复坐标判断

## 本阶段范围

只改 titlebar.c 中 MENU_BTN_HOVER 颜色 + app.c 中 App_OnMouseMove 坐标判断。

## 文件落点

### 本阶段预计修改文件

- src/ui/titlebar.c — 临时改 MENU_BTN_HOVER 为亮红色
- src/app/app.c — 修正 App_OnMouseMove 中按钮 x 坐标范围

### 本阶段原则上不应修改

- 所有其他文件

## 技术路线

### 验证过程

1. 将 MENU_BTN_HOVER 临时改为 (255,80,80) 亮红色
2. 添加 OutputDebugStringW 输出确认 App_OnMouseMove 被调用
3. 发现 App_OnMouseMove 确实被调用，但按钮不变色
4. 检查坐标判断逻辑：原代码使用 frame_visual_thickness 作为 x 起始，按钮实际在右上角
5. 修正为：获取 window_width，用 [width-46, width) 判断 x 范围
6. 移除外亮红色和调试输出，保留正确的坐标判断

### 修正后的代码

```c
void App_OnMouseMove(int x, int y)
{
    unsigned int tmp_width, tmp_height;

    if (g_app.hwnd == NULL || g_app.render == NULL)
        return;

    /* 更新缓存 state：按钮在窗口右上角 [width-46, width) */
    if (App_GetClientSize(&tmp_width, &tmp_height) == 0) {
        int btn_right = (int)tmp_width;
        int btn_left  = (int)tmp_width - 46;
        if (x >= btn_left && x < btn_right &&
            y >= g_app.shell.frame_visual_thickness &&
            y < g_app.shell.frame_visual_thickness + g_app.shell.titlebar_height)
            g_app.menu_button_state = BUTTON_STATE_HOVER;
        else
            g_app.menu_button_state = BUTTON_STATE_NORMAL;
    }

    App_RequestRefresh();
}
```

## 完成标准

1. 编译通过
2. 鼠标移到右上角 ☰ 按钮时颜色变为 hover 色
3. 无 `TMP_` 临时标记残留
4. MENU_BTN_HOVER 已恢复为暖黄色

## 注意

当前 MENU_BTN_HOVER 仍为亮红色 (255,80,80)，需要后续阶段恢复为暖黄色。"变红"链路验证通过，缩回只是恢复颜色值的问题。

## 计划的最后一步

1. git add + commit -m "Shell-2a_repair-7: fix hover hit-test coordinates (was using left-edge instead of right-edge)"
2. git push
3. 完成后重命名为 _done 后缀
