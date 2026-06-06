# Shell-2b_2 — 窗口四边缩放（后续）

## 阶段摘要

在 nonclient.c 的 WM_NCHITTEST 处理中增加四边缩放热区（左/右/上/下），支持鼠标拖拽窗口边缘缩放。四角区域排除在外，留待 Shell-2b_3 处理。

## 阶段目标

1. 在 Platform_Nonclient_HandleNCHitTest 中识别窗口左/右/上/下四条边
2. 对应返回 HTLEFT / HTRIGHT / HTTOP / HTBOTTOM
3. **四边判断排除四角区域**（角落 6x6px 范围不做缩放，留给 Shell-2b_3）
4. 缩放热区宽度固定为 g_frame_resize_thickness（6px）

## 为什么要做这个

1. Shell-2b_1 完成后窗口可拖拽移动，但没有缩放能力
2. 四边缩放先做，四角缩放在 Shell-2b_3 叠加——同一函数内但边界条件独立
3. 四边代码必须排除四角，否则角落会被错误捕获为 HTLEFT/HTTOP

## 阶段产出

1. 鼠标移到窗口左/右/上/下边缘（非角落区域）时出现缩放光标
2. 拖拽窗口边缘可单方向缩放宽度或高度
3. 四角区域（6x6px）不做缩放，留待 Shell-2b_3

## 本阶段范围

只改 nonclient.c 中 Platform_Nonclient_HandleNCHitTest 函数。

## 本阶段不做

1. 不改标题栏命中逻辑
2. 不修改 render 层
3. 不修改窗口默认尺寸和位置

## 分层归属

### platform/win32/nonclient

- Platform_Nonclient_HandleNCHitTest 中增加四边（左/右/上/下）缩放热区判断
- 四边判断通过 y/x 范围限制排除四角 6x6px 区域

## 文件落点

### 本阶段预计修改文件

- src/platform/win32/nonclient.c

### 本阶段原则上不应修改

- src/ui/*、src/app/*、src/render/*、src/storage/*、src/editor/*、src/core/*

## 技术路线

在 Platform_Nonclient_HandleNCHitTest 中插入四边缩放判断，排在标题栏拖拽之前。四边判断通过 y 范围限制排除四角区域（留给 Shell-2b_3）：

```c
    /* ========== 四边缩放热区（排除四角，优先级高于标题栏拖拽） ========== */
    int resize_zone = g_frame_resize_thickness;  /* 6px */

    /* 左边（排除左上角、左下角）*/
    if (x < resize_zone && y >= resize_zone && y < win_h - resize_zone)
        return HTLEFT;
    /* 右边（排除右上角、右下角）*/
    if (x >= win_w - resize_zone && y >= resize_zone && y < win_h - resize_zone)
        return HTRIGHT;
    /* 上边（排除左上角、右上角）*/
    if (y < resize_zone && x >= resize_zone && x < win_w - resize_zone)
        return HTTOP;
    /* 下边（排除左下角、右下角）*/
    if (y >= win_h - resize_zone && x >= resize_zone && x < win_w - resize_zone)
        return HTBOTTOM;
```

## 主链路

```text
鼠标移动到窗口边缘
-> WndProc 收到 WM_NCHITTEST
-> Platform_Nonclient_HandleNCHitTest
   ├─ 四边（排除四角）→ 返回 HTLEFT/右/上/下
   ├─ 标题栏 → 返回 HTCAPTION（菜单按钮区返回 HTCLIENT）
   └─ 其他 → HTCLIENT
-> 系统显示缩放光标 + 启动拖拽缩放
-> 缩放完成 → WM_SIZE → App_OnResize → Render_Resize + Platform_RebuildClientArea
-> 标题栏和正文区跟随重绘
```

## 完成标准

1. 编译通过
2. 鼠标移到窗口左/右/上/下边缘（非角落区域）时出现缩放光标，拖拽可单方向缩放
3. 鼠标移到窗口四角（6x6px 范围）时**不触发缩放**，四角行为留待 Shell-2b_3
4. 缩放后标题栏和正文区正确重绘

## 计划的最后一步

1. git add + commit + push
2. 完成后重命名为 _done 后缀
