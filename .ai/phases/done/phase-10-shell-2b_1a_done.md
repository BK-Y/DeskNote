# Shell-2b_1a — 窗口初始位置设为屏幕右上角（后续）

## 阶段摘要

将窗口启动时的初始位置设为屏幕右上角，使其在桌面角落以便签形态出现。

## 阶段目标

1. 在 Window_Run 或 App_Init 中获取屏幕工作区尺寸
2. 计算窗口位置：x = 屏幕宽度 - 窗口宽度，y = 0（右上角）
3. 替换 CreateWindowExW 中的 CW_USEDEFAULT 为计算后的坐标

## 为什么要做这个

1. CW_USEDEFAULT 让 Windows 自动选择位置，每次启动位置不固定
2. 桌面便签应该出现在屏幕边缘角落，而不是随机位置

## 阶段产出

1. 窗口每次启动都固定在屏幕右上角
2. 窗口不超出屏幕边界（考虑任务栏等）

## 本阶段范围

只改 window.c 中窗口创建时的 x/y 坐标计算。不改窗口尺寸、绘制、布局。

## 本阶段不做

1. 不改窗口尺寸（2b_1 已完成）
2. 不改缩放功能
3. 不改多个显示器的场景

## 分层归属

### platform/win32/window

- 计算屏幕工作区并设置窗口左上角坐标

## 文件落点

### 本阶段预计修改文件

- src/platform/win32/window.c — 替换 CW_USEDEFAULT, CW_USEDEFAULT 为计算坐标

### 本阶段原则上不应修改

- src/ui/*、src/app/*、src/render/*、src/editor/*、src/storage/*、src/core/*

## 技术路线

在 Window_Run 中调用 CreateWindowExW 之前获取屏幕尺寸：

```c
int window_x = CW_USEDEFAULT;
int window_y = CW_USEDEFAULT;

/* 获取屏幕工作区（排除任务栏）*/
RECT work_area;
if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
{
    window_x = (int)(work_area.right - 240);   /* 240 = 窗口宽度 */
    window_y = (int)work_area.top;              /* 右上角 */
}
else
{
    /* 回退到 CW_USEDEFAULT */
    window_x = CW_USEDEFAULT;
    window_y = CW_USEDEFAULT;
}

HWND hwnd = CreateWindowExW(
    0,
    L"DeskNoteWindow",
    L"DeskNote",
    WS_OVERLAPPEDWINDOW,
    window_x, window_y,
    240, 388,
    NULL, NULL, instance, NULL);
```

注意：窗口宽度 240 是硬编码的，如果未来窗口尺寸变化需要同步修改。或者从 `g_app.shell` 或宏定义中读取。

## 主链路

```text
程序启动 -> Window_Run
-> SystemParametersInfoW(SPI_GETWORKAREA) 获取工作区
-> 计算 x = work_area.right - 240, y = work_area.top
-> CreateWindowExW(…, x, y, 240, 388, …)
-> 窗口出现在屏幕右上角
```

## 完成标准

1. 编译通过
2. 启动后窗口固定在屏幕右上角（紧贴右边缘和上边缘）
3. 窗口不超出任务栏区域（在工作区内）

## 计划的最后一步

1. git add + commit + push
2. 完成后重命名为 _done 后缀
