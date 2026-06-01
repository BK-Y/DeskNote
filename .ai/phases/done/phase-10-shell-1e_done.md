# Shell-1e — 通用按钮组件与窗口白色边框修复（后续）

## 阶段摘要

把系统从"标题栏按钮仅为视觉占位、窗口存在白色边框闪烁"推进到"按钮有通用组件支撑可交互、窗口背景由 `Render_Clear` 完全掌控无白边"的状态。

## 阶段目标

1. 创建 `src/ui/button.h` + `src/ui/button.c`，提供可复用的按钮渲染和命中测试接口
2. 修复 `window.c` 中 `hbrBackground` 导致窗口白色边框/闪烁的问题
3. 将 `Shell-1d` 中 TitlebarLayout 的按钮矩形迁移为使用 Button 组件绘制
4. 将标题栏按钮的视觉状态（normal / hover）接入鼠标事件，但不处理点击行为（点击由 Shell-2a 处理）
5. 将 `src/ui/button.c` 和 `src/ui/titlebar.c` 的更新接入 CMakeLists.txt

## 为什么要做这个

1. 白色边框让 frameless 窗体看起来不专业，是窗口壳层基线必须修复的视觉缺陷
2. 当前 Shell-1d 的按钮是硬编码在 Titlebar_Draw 里的 `Render_FillRect` 调用，每次新增按钮或改变视觉都需要修改 titlebar.c，不具备可复用性
3. Shell-2a 需要按钮命中能力，但如果按钮没有独立组件，Shell-2a 的命中逻辑会和绘制逻辑交织在一起，违反分层约束
4. 后续阶段的设置面板、托盘菜单按钮都可以复用同一套按钮组件，不需要各自独立实现

## 阶段产出

1. `src/ui/button.h` 和 `src/ui/button.c` 创建并通过编译
2. 窗口不再有白色边框/背景闪烁（`hbrBackground` 改为 `NULL`）
3. titlebar 中的关闭/最小化按钮改用 Button 组件绘制，视觉一致
4. 鼠标悬停在按钮上时按钮颜色变化（hover 态）
5. Button 组件的命中测试接口可被 Shell-2a 直接消费

## 本阶段范围

1. 新增 `src/ui/button.h` / `button.c`，定义 Button 结构体、绘制、状态更新、命中测试
2. 修改 `src/platform/win32/window.c`，将 `hbrBackground` 设为 `NULL`
3. 修改 `src/ui/titlebar.h` / `titlebar.c`，按钮矩形存储改为 Button 实例数组
4. 修改 `src/app/app.c`，在 `WM_MOUSEMOVE` / `WM_LBUTTONDOWN` 等鼠标事件中调用 `Button_UpdateState` 更新按钮状态
5. 更新 `CMakeLists.txt` 添加 `src/ui/button.c`

## 本阶段不做

1. 不做按钮点击的"命令执行"（关闭窗口/最小化窗口等语义在 Shell-2a）
2. 不做标题栏整体拖拽（Shell-2a）
3. 不做边框缩放（Shell-2b）
4. 不新增 render 层绘制原语
5. 不修改 editor_view / storage / editor / core

## 分层归属

### `ui/button`（新增）

- 定义 Button 结构体和状态枚举
- 提供按钮绘制（调用 render 基础原语）
- 提供命中测试
- 提供鼠标状态更新（normal / hover / pressed）
- 不处理按钮点击的业务语义

**通用性硬约束**：`ui/button.h` 和 `ui/button.c` 中只能包含 `#include "../render/render.h"`。
禁止出现以下任何头文件引用——否则说明 Button 的通用性已被破坏：

- `"../app/app.h"`
- `"../core/"` 下任何头文件
- `"../editor/"` 下任何头文件
- `"../platform/"` 下任何头文件
- `"../storage/"` 下任何头文件

这条约束在代码审查中直接检查 `#include` 列表即可验证，无需理解业务逻辑。

### `ui/titlebar`

- 持有 Button 实例数组，描述标题栏中的每个按钮
- 调用 Button_Init / Button_Draw / Button_UpdateState
- 不直接操作系统窗口行为

### `app/`

- 将平台鼠标事件翻译后传递给 titlebar 的按钮状态更新
- 不直接将平台消息发送给 Button 组件

### `platform/`

- `window.c`：将 `hbrBackground` 设为 `NULL`
- 照常转发鼠标事件给 app

### `render/`

- 不新增原语
- 被 Button_Draw 调用的原语与被 Titlebar_Draw / EditorView_Draw 调用的原语相同

### `editor/` / `storage/` / `core/`

- 本阶段不参与

## 文件落点

### 本阶段新增文件

- `src/ui/button.h` — 通用按钮组件接口
- `src/ui/button.c` — 通用按钮组件实现

### 本阶段预计修改文件

- `src/ui/titlebar.h` — 新增按钮实例数组字段；TitlebarLayout 中按钮矩形改为引用 Button
- `src/ui/titlebar.c` — 改用 Button 组件绘制按钮
- `src/app/app.c` — 在鼠标事件中触发按钮状态更新
- `src/platform/win32/window.c` — `hbrBackground = NULL`
- `CMakeLists.txt` — 添加 `src/ui/button.c`

### 本阶段原则上不应修改

- `src/ui/editor_view.h` / `editor_view.c`
- `src/render/render.h` / `render.c`
- `src/storage/*`
- `src/editor/*`
- `src/core/*`
- `src/platform/win32/nonclient.c`

## 技术路线

### Button 组件接口定义

**`src/ui/button.h`**：

```c
#ifndef DESKNOTE_BUTTON_H
#define DESKNOTE_BUTTON_H

#include "../render/render.h"

typedef enum {
    BUTTON_STATE_NORMAL = 0,
    BUTTON_STATE_HOVER,
    BUTTON_STATE_PRESSED
} ButtonState;

typedef struct {
    RenderRect rect;               // 按钮区域（相对窗口客户区）
    ButtonState state;             // 当前交互状态
    int is_visible;                // 0=隐藏，1=可见
    int is_enabled;                // 0=禁用（不响应鼠标）
    RenderColor bg_color;          // normal 态背景
    RenderColor hover_color;       // hover 态背景
    RenderColor pressed_color;     // pressed 态背景
    const wchar_t* label;          // 标签文字（NULL=无色块文字）
    int label_x;                   // 标签水平偏移（相对 rect.x）
    int label_y;                   // 标签垂直偏移（相对 rect.y）
} Button;

/* 初始化按钮为默认状态 */
void Button_Init(Button* btn, int x, int y, int width, int height);

/* 命中测试：点在按钮区域内返回 1，否则 0 */
int Button_HitTest(const Button* btn, int mx, int my);

/*
 * 根据鼠标位置和按下状态更新按钮交互状态。
 * 应在平台鼠标事件中周期性调用。
 * mouse_down: 当前鼠标左键是否处于按下状态
 */
void Button_UpdateState(Button* btn, int mx, int my, int mouse_down);

/* 绘制按钮（根据当前 state 选择颜色） */
void Button_Draw(RenderContext* ctx, const Button* btn);

#endif
```

**`src/ui/button.c`** 实现要点：
- `Button_Init`：填充 rect，默认 `is_visible=1`、`is_enabled=1`、`state=NORMAL`
- `Button_HitTest`：检查 `(mx >= rect.x && mx < rect.x + rect.width && my >= rect.y && my < rect.y + rect.height)`
- `Button_UpdateState`：
  - 按钮不可见或禁用 → 状态不变
  - 鼠标在按钮内且按下 → PRESSED
  - 鼠标在按钮内且未按下 → HOVER
  - 鼠标不在按钮内 → NORMAL
- `Button_Draw`：
  - 根据 state 选择颜色（PRESED→pressed_color / HOVER→hover_color / NORMAL→bg_color）
  - `Render_FillRect` 画背景
  - 如果有 label，`Render_DrawText` 画文字

### 白色边框修复

**`src/platform/win32/window.c`** - 第 91 行：

```c
// 修改前：
wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

// 修改后：
wc.hbrBackground = NULL;
```

影响：
- `BeginPaint` 不再预填白色背景
- `App_OnPaint` 中的 `Render_Clear` 成为唯一的背景绘制源
- 窗口创建/缩放时不再出现白边闪烁
- 需要确保 `App_OnPaint` 确实绘制了所有像素——当前 `Render_Clear` 是整窗清除，满足此要求

### Titlebar 按钮迁移

**`src/ui/titlebar.h`** — `TitlebarLayout` 结构体变更：

```c
typedef struct {
    RenderRect titlebar_rect;          // 保留
    RenderRect title_text_rect;        // 保留
    Button close_button;               // 新增：替换旧 close_button_rect + has_close_button
    Button minimize_button;            // 新增：替换旧 minimize_button_rect + has_minimize_button
    int window_width;                  // 保留
    int window_height;                 // 保留
} TitlebarLayout;
```

**删除的旧字段**：`close_button_rect`、`minimize_button_rect`、`has_close_button`、`has_minimize_button`。

**`src/ui/titlebar.c`**：
- `Titlebar_CalculateLayout` 中将已有的裸矩形初始化改为 `Button_Init` 调用
- `Titlebar_Draw` 中不再直接 `Render_FillRect` 画按钮，改为调用 `Button_Draw(&layout->close_button)` 和 `Button_Draw(&layout->minimize_button)`
- 颜色配置：关闭按钮 `bg_color=(245,240,200)` `hover_color=(232,80,80)` `pressed_color=(200,60,60)`；最小化按钮 `bg_color=(245,240,200)` `hover_color=(220,215,190)` `pressed_color=(200,195,170)

### App 层鼠标事件接入

**`src/app/app.c`** — 在鼠标事件中更新 titlebar 按钮状态：

```c
void App_OnLeftButtonDown(int x, int y)
{
    // 先更新按钮状态（此时 mouse_down=1 使状态变为 PRESSED）
    // ... 命中 titlebar 按钮 →
    //   Button_UpdateState(&titlebar_layout.close_button, x, y, 1);
    // ... 其余命中逻辑 ...
}

void App_OnMouseMove(int x, int y)
{
    // 更新按钮 hover 状态
    //   Button_UpdateState(&titlebar_layout.close_button, x, y, 0);
    // (需要 g_app 持有当前鼠标位置或从参数获取)
    App_RequestRefresh();  // 触发重绘
}
```

**注意**：当前 `app.h` 中没有 `App_OnMouseMove` 接口，需要在 shell-1e 中新增。或者在 `window.c` 中捕获 `WM_MOUSEMOVE` 并转发给 app。

由于 `window.c` 当前只处理 `WM_LBUTTONDOWN`，需要在 `WindowProc` 中新增 `WM_MOUSEMOVE` 消息处理，调用新的 `App_OnMouseMove(int x, int y)`。

### 旧路径调整

- 旧的 `Titlebar_Draw` 中直接调用 `Render_FillRect` 绘制按钮 → 替换为 `Button_Draw`
- 旧的 `wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1)` → 替换为 `NULL`
- `TitlebarLayout` 中删除 `close_button_rect`、`minimize_button_rect`、`has_close_button`、`has_minimize_button` 四个旧字段 → 替换为 `Button close_button` 和 `Button minimize_button` 两个新字段
- `Titlebar_CalculateLayout` 中原来设置 `has_close_button` / `close_button_rect` 的逻辑 → 改为调用 `Button_Init(&layout->close_button, ...)` + 设置颜色参数
- Shell-1d 的 `TitlebarLayout` 冻结不变，本阶段的字段变更是正常演进，不是回改已完成阶段

## 主链路

```text
程序启动
-> window.c 注册窗口类，hbrBackground=NULL
-> frameless 窗口创建

鼠标移动
-> platform 收到 WM_MOUSEMOVE
-> app 调用 Button_UpdateState (更新为 HOVER/NORMAL)
-> app 触发 WM_PAINT
-> render 绘制：Render_Clear → Titlebar_Draw → Button_Draw → EditorView_Draw

鼠标按下
-> platform 收到 WM_LBUTTONDOWN
-> app 调用 Button_UpdateState (更新为 PRESSED)
-> 不执行按钮命令（Shell-2a）
-> app 触发 WM_PAINT
```

## 完成标准

1. **Button 组件可用** — `src/ui/button.h` / `button.c` 创建，通过编译
2. **白色边框消失** — 设置 `hbrBackground = NULL` 后，窗口启动和 resize 时不再出现白色边框/闪烁
3. **按钮绘制迁移完成** — 标题栏关闭/最小化按钮使用 `Button_Draw` 绘制，视觉显示正常
4. **鼠标悬停响应** — 鼠标悬停在关闭按钮上时按钮颜色变为 hover 色
5. **鼠标点击响应** — 鼠标按下时按钮颜色变为 pressed 色（不执行命令）
6. **构建通过** — `cmake --build build` 无错误无警告
7. **不引入新 render 原语** — 验证 `render.h` 无任何修改

## 计划的最后一步

1. `git add` 当前阶段修改过的所有文件
2. `git commit` 使用规范提交说明
3. `git push` 提交到远端仓库
4. 如果当前阶段确认已完成，把对应计划文件重命名为追加 `_done` 后缀的文件名
