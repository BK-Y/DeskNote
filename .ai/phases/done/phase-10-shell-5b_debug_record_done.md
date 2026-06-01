# 5b_debug_record — WM_SETTINGCHANGE 反馈循环（第2轮）

## 问题描述

EDGE_RESERVED 模式下，窗口右侧出现约一个窗口宽度的死区（240px），且死区不会消失。表现为 `AppBar_SetPosition` 被反复触发，每次把工作区缩小 240px。

## 根因

`AppBar_SetPosition` 调用 `ABM_SETPOS` → Windows 改变工作区 → 异步广播 `WM_SETTINGCHANGE(SPI_SETWORKAREA)`。

在 handler 中，无论尝试什么操作（`AppBar_SetPosition` / `AppBar_ReRegister`），只要触及 `ABM_SETPOS`，就会触发新一轮 `WM_SETTINGCHANGE`，形成无限反馈循环。

## 已尝试但失败的方向

### 方向 A：在 AppBar_SetPosition 加 500ms 节流（第1轮修复）
```
static DWORD s_last_tick = 0;
if (now - s_last_tick < 500) return 0;
```
效果：500ms 内不触发，但只要节流过期，如果有新的 WM_SETTINGCHANGE 在队列中，仍然触发。循环变成 500ms 一次。

### 方向 B：在 WM_SETTINGCHANGE handler 加 500ms 节流
效果：同上，500ms 节流到期后，仍然重新进入循环。

### 方向 C：完全移除 WM_SETTINGCHANGE 对 AppBar 的操作
效果：外部工作区变化（用户拖动任务栏）时无法响应。

## 最终修复（方向 D：状态标记）

原则：用"谁触发了"的状态判断，而不是时间节流。

```c
// AppBar_SetPosition 中，ABM_SETPOS 之前：
AppBar_MarkOwnWorkareaChange();
SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

// WM_SETTINGCHANGE handler 中：
if (AppBar_ConsumeOwnWorkareaChange()) {
    // 我们自己触发的，跳过
    return 0;
}
// 外部触发，正常处理
if (AppBar_IsRegistered(hwnd))
    AppBar_ReRegister(hwnd);
```

`MarkOwnWorkareaChange` 设置一个标记，`ConsumeOwnWorkareaChange` 读取并清除该标记。WM_SETTINGCHANGE 是异步投递的，但标记在 AppBar_SetPosition 中设置后，在下一次 `ConsumeOwnWorkareaChange` 消费之前一直有效。由于 ABM_SETPOS 触发的 WM_SETTINGCHANGE 总是紧跟在当前消息之后到达，标记一定还在。

## 涉及文件

- `src/platform/win32/appbar.h` — 新增 `MarkOwnWorkareaChange` / `ConsumeOwnWorkareaChange` 声明
- `src/platform/win32/appbar.c` — 实现两个函数；`AppBar_SetPosition` 中 ABM_SETPOS 前调用 `MarkOwnWorkareaChange`
- `src/platform/win32/window.c` — `WM_SETTINGCHANGE` handler 中使用 `ConsumeOwnWorkareaChange` 判断
