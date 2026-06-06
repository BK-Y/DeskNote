# Shell-2b_3 — 窗口四角缩放（后续）

## 阶段摘要

在 nonclient.c 的 WM_NCHITTEST 处理中增加四角缩放热区（左上/右上/左下/右下），覆盖 Shell-2b_2 特意排除的四角区域。

## 阶段目标

1. 在 Platform_Nonclient_HandleNCHitTest 中识别窗口四个角落（6x6px 范围）
2. 对应返回 HTTOPLEFT / HTTOPRIGHT / HTBOTTOMLEFT / HTBOTTOMRIGHT
3. 四角判断放在四边判断之前，确保角落缩放优先于边缘缩放

## 为什么要做这个

1. Shell-2b_2 的四边代码排除了角落区域，角落 6x6px 范围目前不触发任何缩放
2. 四角缩放可以同时改变窗口宽度和高度，是桌面窗口的标准交互
3. 四角与四边的判断条件不同（四角同时约束 x 和 y），但同属一个函数，需要在四边之前插入

## 阶段产出

1. 鼠标移到窗口四角时出现对角缩放光标
2. 拖拽四角可从两个方向同时缩放窗口宽度和高度
3. 角落缩放优先于边缘缩放

## 本阶段范围

只改 nonclient.c 中 Platform_Nonclient_HandleNCHitTest 函数，在四边判断之前插入四角判断。

## 本阶段不做

1. 不重复四边缩放逻辑
2. 不改标题栏命中逻辑
3. 不修改 render 层

## 分层归属

### platform/win32/nonclient

- Platform_Nonclient_HandleNCHitTest 中在四边判断之前插入四角判断

## 文件落点

### 本阶段预计修改文件

- src/platform/win32/nonclient.c

### 本阶段原则上不应修改

- src/ui/*、src/app/*、src/render/*、src/storage/*、src/editor/*、src/core/*

## 技术路线

在 Platform_Nonclient_HandleNCHitTest 中，在四边判断之前插入四角判断：

```c
    /* ========== 四角缩放热区（优先于四边） ========== */
    int resize_zone = g_frame_resize_thickness;  /* 6px */

    if (x < resize_zone && y < resize_zone)
        return HTTOPLEFT;
    if (x >= win_w - resize_zone && y < resize_zone)
        return HTTOPRIGHT;
    if (x < resize_zone && y >= win_h - resize_zone)
        return HTBOTTOMLEFT;
    if (x >= win_w - resize_zone && y >= win_h - resize_zone)
        return HTBOTTOMRIGHT;

    /* 后面是 Shell-2b_2 已实现的四边判断（对 y 有范围限制，排除了角落）*/
    /* 左边 */
    if (x < resize_zone && y >= resize_zone && y < win_h - resize_zone)
        return HTLEFT;
    ...
```

注意：四角判断的条件比四边更严格（同时检查 x 和 y），且放在四边之前。Shell-2b_2 的四边代码已经通过 y 范围排除了角落区域，所以四角的插入不会与现有四边逻辑冲突，只是覆盖了之前留空的区域。

## 主链路

```text
鼠标移动到窗口角落
-> WndProc 收到 WM_NCHITTEST
-> Platform_Nonclient_HandleNCHitTest
-> 四角判断优先 → 返回 HTTOPLEFT/右上/左下/右下
-> 系统显示对角缩放光标 + 启动拖拽缩放
-> 缩放完成 → WM_SIZE → App_OnResize → Render_Resize + Platform_RebuildClientArea
-> 标题栏和正文区跟随重绘
```

## 完成标准

1. 编译通过
2. 鼠标移到窗口四角时出现对角缩放光标
3. 拖拽四角可从两个方向同时缩放宽度和高度
4. 四边缩放仍然正常，不受四角代码影响

## 计划的最后一步

1. git add + commit + push
2. 完成后重命名为 _done 后缀
