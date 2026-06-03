# Shell-5d — 标题栏状态指示灯

## ① 核心问题

切换浮动置顶或贴边占位后，用户需要点开菜单或 hover 托盘图标才能确认当前模式。标题栏上没有任何视觉标记。

## ② 目标

在标题栏右侧（汉堡菜单按钮旁边）增加一个状态指示灯按钮，与现有 `Button` 组件共用渲染基础设施。通过按钮底色或符号区分模式。

## ③ 方案设计

在 `TitlebarLayout` 中新增 `Button status_indicator`，在标题栏布局计算时定位到汉堡包按钮左侧，在 `Titlebar_Draw` 中绘制。

指示灯不需要交互（不响应点击），只做视觉展示。

显示方式：使用圆点符号 `●` 或几何图形，通过 `label_color` 区分模式。

## ④ 具体修改

### 4.1 结构体

```c
/* src/ui/titlebar.h */
typedef struct {
    RenderRect titlebar_rect;
    RenderRect title_text_rect;
    Button menu_button;
    Button status_indicator;       /* Shell-5d */
    int window_width;
    int window_height;
} TitlebarLayout;

/* 新增：刷新指示灯外观 */
void Titlebar_UpdateStatus(Button* indicator, AppShellResidentMode mode);
```

### 4.2 布局计算

在 `TitlebarLayout` 计算时，在 `menu_button` 左边留出 20x20 区域：

```c
/* 在标题栏现有布局代码中，menu_button 初始化之后追加 */
Button_Init(&layout->status_indicator,
    layout->menu_button.rect.x - 28,           /* 在菜单按钮左侧，留 8px 间距 */
    (titlebar_height - 20) / 2,                /* 垂直居中 */
    20, 20);
```

### 4.3 外观更新——在 `App_OnPaint` 中设置

与 `menu_button.state` 更新方式一致，在布局计算后、绘制前追加：

```c
/* app.c App_OnPaint */
titlebar_layout = Titlebar_CalculateLayout(...);
titlebar_layout.menu_button.state = g_app.menu_button_state;
Titlebar_UpdateStatus(&titlebar_layout.status_indicator, g_app.shell.resident_mode);  /* 新增 */
Titlebar_Draw(g_app.render, &titlebar_layout);
```

`Titlebar_UpdateStatus` 接收一个 `Button*` 指针，直接修改其外观字段（`bg_color`、`label`、`is_visible`），不依赖 `App_GetResidentMode()`、不反向依赖 `app.h`：

```c
void Titlebar_UpdateStatus(Button* indicator, AppShellResidentMode mode)
{
    switch (mode)
    {
    case APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST:
        indicator->bg_color = (RenderColor){255, 200, 0, 255};
        indicator->label = L"●";
        break;
    case APP_SHELL_RESIDENT_MODE_EDGE_RESERVED:
        indicator->bg_color = (RenderColor){0, 150, 255, 255};
        indicator->label = L"●";
        break;
    default:
        indicator->bg_color = (RenderColor){0, 0, 0, 0};
        indicator->label = L"";
        break;
    }
    indicator->is_visible = (mode != APP_SHELL_RESIDENT_MODE_NONE);
}
```

不需要在 `App_UpdateTrayTip` 调用点追加任何东西。`App_RequestRefresh()` 已在各处调用，触发重绘后 `App_OnPaint` 自动更新指示灯。

## ⑤ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/ui/titlebar.h` | `TitlebarLayout` 新增 `status_indicator`；声明 `Titlebar_UpdateStatus` |
| `src/ui/titlebar.c` | 布局计算追加 `status_indicator` 定位；`Titlebar_Draw` 追加绘制；实现 `Titlebar_UpdateStatus` |
| `src/app/app.c` | `App_OnPaint` 中追加一行 `Titlebar_UpdateStatus` 调用 |

不应修改：`src/storage/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/platform/win32/appbar.*`。

## ⑥ 验收标准

1. NONE 模式：标题栏无指示点
2. FLOATING_TOPMOST：菜单按钮左侧出现橙色圆点
3. EDGE_RESERVED：菜单按钮左侧出现蓝色圆点
4. 切换模式时圆点颜色即时变化
5. 圆点不可点击（不响应鼠标事件）

## ⑦ 分层核实

- `titlebar.h/c` 属于 `ui/` 层，负责布局和绘制
- `App_RequestRefresh()` 触发重绘，不直接操作平台
- `Titlebar_UpdateStatus` 仅修改 Button 字段，不调用系统 API
- 不修改 `render/*`、`storage/*`、`platform/win32/appbar.*`
