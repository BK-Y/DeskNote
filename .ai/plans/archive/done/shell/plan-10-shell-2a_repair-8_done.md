# Shell-2a_repair-8 — 修复鼠标移出窗口后按钮 hover 状态残留（后续）

## 阶段摘要

修复当鼠标从菜单按钮快速移出到其他窗口时，按钮 hover 亮红色状态不会恢复为 normal 的问题。

## 阶段目标

1. 在 window.c 中注册 TrackMouseEvent 以接收 WM_MOUSELEAVE 消息
2. 在 window.c 的 WndProc 中处理 WM_MOUSELEAVE，调用 App_OnMouseLeave()
3. 在 app.c/app.h 中实现 App_OnMouseLeave：将 menu_button_state 重置为 NORMAL 并触发重绘
4. 每次收到 WM_MOUSEMOVE 时重新注册 TrackMouseEvent（一次性事件）

## 为什么要做这个

1. 当前鼠标移出窗口到其他软件时，最后一个 HOVER state 不会恢复为 NORMAL，按钮一直保持 hover 色
2. Windows 不会在鼠标离开窗口时自动通知应用，必须通过 TrackMouseEvent 主动请求 WM_MOUSELEAVE 通知
3. 这个问题不修，按钮状态在跨窗口场景下永远处于错误状态

## 本阶段产出

1. 鼠标从窗口内移动到其他窗口时，按钮立即恢复为正常颜色
2. 鼠标在窗口内移动时 hover 行为不受任何影响

## 本阶段范围

1. window.c 中 WndProc 新增 WM_MOUSELEAVE case
2. window.c 中 WM_MOUSEMOVE case 新增 TrackMouseEvent 调用
3. app.h 新增 App_OnMouseLeave 声明
4. app.c 新增 App_OnMouseLeave 实现

## 本阶段不做

1. 不修改 button.h/c
2. 不修改 titlebar.h/c
3. 不修改 render
4. 不做鼠标捕获（SetCapture）相关改动

## 分层归属

### platform/win32/window.c

- WM_MOUSEMOVE 处理中调用 TrackMouseEvent 注册离开跟踪
- WndProc 中新增 WM_MOUSELEAVE 处理，转发给 app

### app/

- app.h 新增 App_OnMouseLeave 声明
- app.c 实现 App_OnMouseLeave：重置 state + 刷新

### ui/ / render/ / editor/ / storage/ / core/

- 本阶段不参与

## 文件落点

### 本阶段预计修改文件

- src/platform/win32/window.c — TrackMouseEvent 调用 + WM_MOUSELEAVE 处理
- src/app/app.h — 新增 void App_OnMouseLeave(void) 声明
- src/app/app.c — 实现 App_OnMouseLeave

### 本阶段原则上不应修改

- src/ui/*、src/render/*、src/editor/*、src/storage/*、src/core/*

## 技术路线

### window.c

WndProc 中 WM_MOUSEMOVE 处理：

```c
case WM_MOUSEMOVE:
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hwnd;
    TrackMouseEvent(&tme);
    App_OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
}
```

WndProc 中新增 WM_MOUSELEAVE 处理：

```c
case WM_MOUSELEAVE:
    App_OnMouseLeave();
    return 0;
```

注意：#include <commctrl.h> 可能已包含 TrackMouseEvent，如缺少则需 #include <windowsx.h>。

### app.h

```c
void App_OnMouseLeave(void);
```

### app.c

```c
void App_OnMouseLeave(void)
{
    g_app.menu_button_state = BUTTON_STATE_NORMAL;
    App_RequestRefresh();
}
```

## 主链路

```text
鼠标移出窗口
→ 系统检测到鼠标离开（通过之前注册的 TrackMouseEvent）
→ 发送 WM_MOUSELEAVE
→ WndProc 收到消息，调用 App_OnMouseLeave()
→ 将 menu_button_state 置为 NORMAL
→ App_RequestRefresh 触发 WM_PAINT
→ 下一次 App_OnPaint 中 state = NORMAL → Button_Draw 选 bg_color
→ 按钮恢复为正常颜色
```

## 完成标准

1. 编译通过，0 error 0 warning
2. 鼠标从菜单按钮直接移出到其他窗口后，按钮颜色立即恢复为默认色
3. 鼠标在窗口内移动时，hover 效果正常，不影响进出按钮区域

## 计划的最后一步

1. git add + commit -m "Shell-2a_repair-8: reset hover state on WM_MOUSELEAVE"
2. git push
3. 完成后重命名为 _done 后缀
