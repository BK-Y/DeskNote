# Shell-5a_repair-3 — 测试级：诊断贴边宽度实际值（后续）

## ① 核心问题

枚举值已修复（repair-2），但需要运行时验证 `ABM_SETPOS` 返回的 AppBar 矩形和 `MoveWindow` 的宽度是否与预期 240px 一致。这是**测试级修复**——只加诊断输出，不改变业务逻辑。

## ② 分析现状

修复后的代码已对齐 `ABE_*` 常量，但运行时行为需要验证。

需要确认的关键值：
1. `SPI_GETWORKAREA` 返回的原始工作区
2. 我们自己设置的 rc 宽度（`rc.right - 240`）
3. `ABM_QUERYPOS` 之后系统调整的 rc
4. `ABM_SETPOS` 之后系统最终确认的 rc
5. `MoveWindow` 实际传入的宽高

## ③ 方案设计

只有一条合理路径：在 `AppBar_SetPosition` 中关键位置插入诊断输出，用 `OutputDebugStringW` 写入调试日志，用 DebugView 或 VS 输出窗口查看。不改任何业务逻辑。

所有添加的代码行尾标记 `// phase-10-shell-5a-repair-3`，测试完成后根据日志结论决定保留或回退。修改的代码行在修改处也标注。

## ④ 拆解执行

### 整体目标

1. 在 `AppBar_SetPosition` 中插入 5 个诊断输出点
2. 编译运行，用 DebugView 抓取日志
3. 根据日志确认宽度是否为 240px
4. 测试完成后回退所有诊断代码

### 阶段产出

运行日志显示贴边宽度实际值，用于判断是否需要进一步修复。

## ⑤ 设定边界

### 本阶段范围

只在 `AppBar_SetPosition` 中加诊断输出。不改变任何逻辑。

### 不包括

1. 不改业务逻辑
2. 不改其他文件
3. 不改变任何 API 调用参数

### 文件落点

#### 预计修改文件

- `src/platform/win32/appbar.c` — `AppBar_SetPosition` 函数内加诊断输出

## ⑥ 落地方案

### 技术路线

在 `AppBar_SetPosition` 的 5 个关键位置插入 `OutputDebugStringW` + `swprintf` 组合：

```c
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 1 — 原始工作区 ---- */
    {
        wchar_t buf[128];
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),
            L"[Shell-5a-repair-3] workarea: L=%d T=%d R=%d B=%d\r\n",
            rc.left, rc.top, rc.right, rc.bottom);
        OutputDebugStringW(buf);
    }

    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;

    switch (edge) { /* ... 不变 ... */ }

    /* ---- phase-10-shell-5a-repair-3: 诊断输出 2 — 我们自己设置的 rc ---- */
    {
        wchar_t buf[128];
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),
            L"[Shell-5a-repair-3] before ABM_QUERYPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
            s_appbar.data.rc.left, s_appbar.data.rc.top,
            s_appbar.data.rc.right, s_appbar.data.rc.bottom,
            s_appbar.data.rc.right - s_appbar.data.rc.left,
            s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    SHAppBarMessage(ABM_QUERYPOS, &s_appbar.data);

    /* ---- phase-10-shell-5a-repair-2: 诊断输出 3 — QUERYPOS 之后 ---- */
    {
        wchar_t buf[128];
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),
            L"[Shell-5a-repair-3] after ABM_QUERYPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
            s_appbar.data.rc.left, s_appbar.data.rc.top,
            s_appbar.data.rc.right, s_appbar.data.rc.bottom,
            s_appbar.data.rc.right - s_appbar.data.rc.left,
            s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    /* ---- phase-10-shell-5a-repair-2: 诊断输出 4 — SETPOS 之后 ---- */
    {
        wchar_t buf[128];
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),
            L"[Shell-5a-repair-3] after ABM_SETPOS: L=%d T=%d R=%d B=%d (w=%d h=%d)\r\n",
            s_appbar.data.rc.left, s_appbar.data.rc.top,
            s_appbar.data.rc.right, s_appbar.data.rc.bottom,
            s_appbar.data.rc.right - s_appbar.data.rc.left,
            s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    /* ---- phase-10-shell-5a-repair-2: 诊断输出 5 — 最终 MoveWindow 参数 ---- */
    {
        wchar_t buf[128];
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),
            L"[Shell-5a-repair-3] MoveWindow: x=%d y=%d w=%d h=%d\r\n",
            s_appbar.data.rc.left, s_appbar.data.rc.top,
            s_appbar.data.rc.right - s_appbar.data.rc.left,
            s_appbar.data.rc.bottom - s_appbar.data.rc.top);
        OutputDebugStringW(buf);
    }

    MoveWindow(hwnd, /* ... 不变 ... */);

    return 0;
}
```

**可修复性注释：** 所有 phase-10-shell-5a-repair-2 标记的代码块在测试完成后整体删除。标记格式为 `| 可回退 |` 注释 + 代码块边界标记。

## ⑦ 验收标准

1. 编译通过
2. 运行时在 DebugView 中看到 5 行 `[Shell-5a-repair-3]` 日志
3. MoveWindow 的宽度行显示 `w=240`
4. 如果宽度 ≠ 240，则 QUERYPOS/SETPOS 日志揭示被调整的位置，据此决定下一步修复

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/platform/win32/appbar.c` 的 `AppBar_SetPosition` 中插入 5 个诊断输出块，每行标记 `// phase-10-shell-5a-repair-2`——运行时输出关键矩形的值，不改逻辑 | 诊断代码就位 | 编译通过 |
| 2 | 运行程序，点击"贴边占位"，用 DebugView 捕获日志 | 日志输出完整 | 5 行日志全部出现 |
| 3 | 分析日志中 `MoveWindow: w=???` 的值——确认是否为 240 | 明确宽度根因 | 根据日志结论决定下一步 |
| 4 | 测试完成后，删除所有 `phase-10-shell-5a-repair-3` 标记的诊断代码——回退到 repair-2 的纯净状态 | 诊断代码已清理 | 编译通过 |
