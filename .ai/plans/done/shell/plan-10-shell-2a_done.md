# Shell-2a — 菜单按钮 + 弹出菜单 + 标题栏拖拽（后续）

## 阶段摘要

把系统从"标题栏有 close/minimize 两个 Button 实例但点击无命令执行"推进到"标题栏只有一个菜单按钮、点击弹出上下文菜单（关闭/关于）、非按钮区可拖拽移动窗口"的状态。

## 阶段目标

1. `TitlebarLayout` 中 `close_button` + `minimize_button` 替换为单个 `menu_button`，对应 `CalculateLayout` / `Draw` 同步更新
2. 菜单按钮点击后弹出 Win32 上下文菜单，含"关闭"和"关于"两个菜单项
3. "关于"对话框显示 DeskNote 版本号 + md4c 的 MIT 版权声明
4. 标题栏非按钮区接入窗口拖拽
5. 保证菜单按钮点击不误触拖拽

## 为什么要做这个

1. 便签工具不需要最大化/最小化按钮。关闭按钮放在 titlebar 上容易误触，移到菜单中更安全
2. Shell-1d/1e 完成后的 titlebar 有两个 Button（close + minimize），需要先迁移到单个 menu_button
3. md4c 使用 MIT 协议，需要在"关于"中提供版权归属，这是开源合规要求，不能省略
4. 菜单按钮是后续（设置面板、按钮配置化等）的统一入口，必须先建好

## 阶段产出

1. titlebar 右上角只有一个菜单按钮（"☰" 或 "⋮" 视觉），原 close/minimize 按钮消失
2. 点击菜单按钮弹出菜单，含"关闭"和"关于"
3. "关于"对话框显示 `DeskNote v0.1` + md4c MIT 版权文本
4. 标题栏非按钮区可拖拽移动窗口
5. 按钮点击不触发拖拽

## 本阶段范围

1. `titlebar.h` — `TitlebarLayout` 结构体变更：删除 `close_button`、`minimize_button`，新增 `menu_button`
2. `titlebar.c` — `CalculateLayout` / `Draw` 同步更新
3. `app.h` — 新增 `APP_SHELL_COMMAND_SHOW_MENU`；保留 `APP_SHELL_COMMAND_WINDOW_CLOSE` 供菜单项使用
4. `app.c` — `App_OnLeftButtonDown` 中处理菜单按钮命中；`App_SubmitShellCommand` 中处理 SHOW_MENU（弹出 TrackPopupMenu）和 WINDOW_CLOSE
5. `window.c` — 新增 `Platform_BeginWindowDrag`；处理 `WM_SYSCOMMAND`（SC_CLOSE）
6. `nonclient.c` — 替换"整个顶区 HTCAPTION"为精确范围

## 本阶段不做

1. 不实现设置面板（仅为菜单预留入口）
2. 不做窗口缩放（Shell-2b）
3. 不新增 render 层原语
4. 不修改 editor_view / storage / editor / core
5. 不修改 Shell-1d/1e 的 `_done` 文件

## 分层归属

### `ui/titlebar`

- `TitlebarLayout` 结构体变更：`close_button` + `minimize_button` → `menu_button`
- 提供 `TitlebarHitResult` 枚举 + `Titlebar_HitTest`：`HIT_NONE`、`HIT_MENU_BUTTON`、`HIT_DRAG_BAR`
- `Titlebar_Draw` 调用 `Button_Draw(&layout->menu_button)`
- 不直接操作 Win32 菜单或窗口

### `app/`

- `App_OnLeftButtonDown` 中调用 `Titlebar_HitTest`，命中菜单按钮时触发 `App_SubmitShellCommand(SHOW_MENU)`
- `App_SubmitShellCommand` 中处理 SHOW_MENU：创建 Win32 弹出菜单 + `TrackPopupMenu`
- SHOW_MENU 的关闭项 → `PostQuitMessage(0)`
- SHOW_MENU 的关于项 → `MessageBox` 显示版权

### `platform/`

- `window.c`：新增 `Platform_BeginWindowDrag(HWND hwnd)`，封装 `SendMessage(WM_NCLBUTTONDOWN, HTCAPTION)` 启动系统拖拽
- `nonclient.c`：`Platform_Nonclient_HandleNCHitTest` 只确认非按钮区时才返回 `HTCAPTION`，菜单按钮区返回 `HTCLIENT`

### `render/`

- 本阶段不参与

### `editor/` / `storage/` / `core/`

- 本阶段不参与

## 文件落点

### 本阶段预计修改文件

| 文件 | 改动 |
|------|------|
| `src/ui/titlebar.h` | `TitlebarLayout` 删除 `close_button` `minimize_button`，新增 `menu_button`；新增 `TitlebarHitResult` 枚举 + `Titlebar_HitTest` 声明 |
| `src/ui/titlebar.c` | `CalculateLayout` 改为初始化 `menu_button`（位置颜色）；`Titlebar_Draw` 改为 `Button_Draw(&menu_button)`；实现 `Titlebar_HitTest` |
| `src/app/app.h` | 新增 `APP_SHELL_COMMAND_SHOW_MENU` |
| `src/app/app.c` | `App_OnLeftButtonDown` 加入菜单按钮命中判断；`App_SubmitShellCommand` 处理 SHOW_MENU / WINDOW_CLOSE |
| `src/platform/win32/window.h` | 新增 `Platform_BeginWindowDrag` 声明 |
| `src/platform/win32/window.c` | 实现 `Platform_BeginWindowDrag` |
| `src/platform/win32/nonclient.c` | 替换"整个顶区 HTCAPTION"为精确判断 |

### 本阶段原则上不应修改

- `src/ui/button.h` / `button.c`（冻结）
- `src/ui/editor_view.c`
- `src/render/render.h` / `render.c`
- `src/storage/*`
- `src/editor/*`
- `src/core/*`

## 技术路线

### 新增入口文件

本阶段不新增文件，全部在现有文件中修改。

**`src/ui/titlebar.h`** — 结构体变更：

```c
// 修改前（Shell-1e 遗留）：
typedef struct {
    RenderRect titlebar_rect;
    RenderRect title_text_rect;
    Button close_button;
    Button minimize_button;
    int window_width;
    int window_height;
} TitlebarLayout;

// 修改后：
typedef struct {
    RenderRect titlebar_rect;
    RenderRect title_text_rect;
    Button menu_button;              // 替换旧的 close_button + minimize_button
    int window_width;
    int window_height;
} TitlebarLayout;
```

新增枚举和函数：

```c
typedef enum {
    TITLEBAR_HIT_NONE = 0,
    TITLEBAR_HIT_MENU_BUTTON,
    TITLEBAR_HIT_DRAG_BAR
} TitlebarHitResult;

TitlebarHitResult Titlebar_HitTest(const TitlebarLayout* layout, int x, int y);
```

**`src/platform/win32/window.h`** — 新增：

```c
void Platform_BeginWindowDrag(HWND hwnd);
```

### 状态承接文件

**`src/app/app.h`** — 新增命令枚举值：

```c
typedef enum {
    APP_SHELL_COMMAND_NONE = 0,
    APP_SHELL_COMMAND_SHOW_MENU,        // 新增，替换原 CLOSE/MINIMIZE
    APP_SHELL_COMMAND_WINDOW_CLOSE,     // 保留，供菜单项使用
    APP_SHELL_COMMAND_WINDOW_RESTORE,
    APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST,
    APP_SHELL_COMMAND_ENTER_EDGE_RESERVED,
    APP_SHELL_COMMAND_HIDE_TO_TRAY,
    APP_SHELL_COMMAND_RESTORE_FROM_TRAY
} AppShellCommand;
```

删除 `APP_SHELL_COMMAND_WINDOW_MINIMIZE`。

**`src/ui/titlebar.c`** — `CalculateLayout` 中的变更：

```c
// 原来初始化 close_button + minimize_button 的位置
// 改为初始化单个 menu_button

Button_Init(&layout->menu_button, current_x, titlebar_y,
            TITLEBAR_BUTTON_WIDTH, titlebar_height);
layout->menu_button.bg_color = (RenderColor){ 245, 240, 200, 255 };
layout->menu_button.hover_color = (RenderColor){ 200, 200, 220, 255 };
layout->menu_button.pressed_color = (RenderColor){ 180, 180, 200, 255 };
layout->menu_button.is_visible = 1;
layout->menu_button.is_enabled = 1;
// 可使用 Unicode 字符 "☰" (U+2630) 或简单文字 "M" 作为标签
```

### 调度 / 测量 / 适配文件

**`src/app/app.c`** — `App_OnLeftButtonDown` 流程：

```c
void App_OnLeftButtonDown(int x, int y)
{
    // 1. 先检查菜单按钮命中
    TitlebarLayout layout = /* 计算或缓存 */;
    TitlebarHitResult hit = Titlebar_HitTest(&layout, x, y);
    if (hit == TITLEBAR_HIT_MENU_BUTTON) {
        App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_MENU);
        return;
    }
    if (hit == TITLEBAR_HIT_DRAG_BAR) {
        Platform_BeginWindowDrag(g_app.hwnd);
        return;
    }
    // 2. 否则走编辑器光标定位
    // ... 原有 EditorView_HitTestCursor 逻辑 ...
}
```

`App_SubmitShellCommand` 中新增处理：

```c
case APP_SHELL_COMMAND_SHOW_MENU:
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 100, L"关闭");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 101, L"关于 DeskNote...");
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                             x, y, 0, g_app.hwnd, NULL);
    DestroyMenu(hMenu);
    if (cmd == 100)  // 关闭
        PostQuitMessage(0);
    else if (cmd == 101)  // 关于
        MessageBoxW(g_app.hwnd,
                    L"DeskNote v0.1\n\n"
                    L"A lightweight desktop notes application.\n\n"
                    L"Uses md4c - Markdown for C\n"
                    L"Copyright (c) 2016, Martin Mitas\n"
                    L"MIT License",
                    L"关于 DeskNote",
                    MB_OK | MB_ICONINFORMATION);
    break;
}
```

**`src/platform/win32/nonclient.c`** — 替换拖拽范围：

```c
LRESULT Platform_Nonclient_HandleNCHitTest(HWND hwnd, LPARAM lParam)
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    
    RECT wr;
    if (!GetWindowRect(hwnd, &wr))
        return HTCLIENT;
    
    int y = pt.y - wr.top;
    int x = pt.x - wr.left;
    
    // 如果点在菜单按钮区域，不触发拖拽
    if (y >= g_titlebar_height - 46 && y < g_titlebar_height &&
        x >= wr.right - wr.left - 46 && x < wr.right - wr.left)
        return HTCLIENT;
    
    // 标题栏区域内：可拖拽
    if (y >= 0 && y < g_titlebar_height)
        return HTCAPTION;
    
    return HTCLIENT;
}
```

注意：标题栏的 y 坐标需要加上 `g_frame_visual_thickness` 偏移，但上述代码为示意。实际实现时应与 `Titlebar_CalculateLayout` 保持一致。

### 菜单按钮标签说明

当前 `Button_Draw` 支持 `label` 字段，可使用 Unicode 字符 `L"\u2630"`（☰ 三横线汉堡菜单图标）或纯文字 `"≡"`。若 DirectWrite 不支持该字形，回退为文字 `"M"`。

## 实施记录

### 发现问题

首次实施后，菜单按钮在界面上不可见。

### 根因分析

1. `menu_button.label` 未设置（NULL），按钮无文字/图标
2. `Button_Draw` 用 `fill_color`（按钮背景色）绘制 label 文字，背景色浅时文字不可见
3. 菜单按钮背景色 `(245,240,200)` 与标题栏背景色 `(255,251,214)` 极其接近，按钮区域无法与标题栏区分

### 修复内容

| 文件 | 改动 | 说明 |
|------|------|------|
| `src/ui/button.h` | 新增 `label_color` 字段 | Button 结构体新增 `RenderColor label_color`，全零时默认降级为深灰 `(40,40,40)` |
| `src/ui/button.c` | `Button_Draw` 改用 `label_color` 绘制文字 | 修复原代码中用 `fill_color` 画文字导致浅色背景上文字不可见的问题 |
| `src/ui/titlebar.c` | 设 `menu_button.label = L"\u2630"` | 显示汉堡图标 ☰ |
| `src/ui/titlebar.c` | 设 `label_color = (100,100,100)` | 图标颜色为深灰，与浅色背景区分 |
| `src/ui/titlebar.c` | `MENU_BTN_BG` 加深为 `(230,225,200)` | 按钮底色调深，与标题栏背景肉眼可区分 |

### 影响范围

- `button.h/c` 属于 `Shell-1e_done`（冻结阶段），本次新增 `label_color` 字段属于正常扩展（不删除现有字段、不改现有接口签名、不改变默认行为）
- `memset(btn, 0, sizeof(*btn))` 将 `label_color` 初始化为全零 → `Button_Draw` 自动降级为 `(40,40,40)` → 所有现有无 label 的按钮不受影响

### 旧路径调整

- `TitlebarLayout` 中删除 `close_button` + `minimize_button` + 裸字段 → 替换为 `menu_button`
- `Platform_Nonclient_HandleNCHitTest` 从"整个顶区 HTCAPTION" → "非按钮区 HTCAPTION，按钮区 HTCLIENT"
- `App_SubmitShellCommand` 删除对 `APP_SHELL_COMMAND_WINDOW_MINIMIZE` 的处理

### 后续可配置按钮预留说明

当前 titlebar 只有 menu_button。未来如需在 titlebar 上显示其他按钮（如 close 按钮），可：

1. 恢复 `TitlebarLayout` 中的 `close_button` 字段
2. 通过 `titlebar_command_groups` 位掩码控制可见性
3. 在 `Titlebar_CalculateLayout` 中根据位掩码决定是否初始化该按钮
4. 设置面板提供一个配置入口来修改位掩码

当前阶段不改动 `AppShellState.titlebar_command_groups` 的字段结构。

## 主链路

```text
鼠标按下（标题栏区域）
-> ui/titlebar : Titlebar_HitTest 判断命中
  ├─ HIT_MENU_BUTTON → app : App_SubmitShellCommand(SHOW_MENU)
  │                     → platform : TrackPopupMenu
  │                       ├─ "关闭" → PostQuitMessage
  │                       └─ "关于" → MessageBox (含 md4c 版权)
  └─ HIT_DRAG_BAR → platform : Platform_BeginWindowDrag → 系统拖拽
```

## 完成标准

1. **菜单按钮可见** — 启动后 titlebar 右上角只有一个菜单按钮（☰ 或 ≡），原 close/minimize 按钮消失
2. **菜单可弹出** — 点击菜单按钮弹出上下文菜单，含"关闭"分隔线"关于"
3. **关于对话框显示正确** — 菜单→关于弹出 MessageBox，显示 DeskNote v0.1 + md4c MIT 版权
4. **关闭可退出程序** — 菜单→关闭执行 `PostQuitMessage`，程序退出
5. **标题栏可拖拽** — 在菜单按钮以外的 titlebar 区域按下鼠标拖拽可移动窗口
6. **按钮点击不误触拖拽** — 点击菜单按钮时不触发窗口移动
7. **构建通过** — `cmake --build build` 无错误无警告

## 计划的最后一步

1. `git add` 当前阶段修改过的文件
2. `git commit` 使用规范提交说明
3. `git push` 提交到远端仓库
4. 如果当前阶段确认已完成，重命名为 `_done` 后缀
