# Rollback-5a — 回退 Shell-5a 所有未确认修复，重新诊断

## 回退目标

将 appbar.h、appbar.c、window.c、app.c 四个文件回退到 Shell-5a 原始提交状态（1ad1d12），其上只叠加两样东西：

1. **repair-2 的枚举修正** — `AppDockEdge` 值对齐 `ABE_*`（已确认正确）
2. **2 行诊断输出** — 用于在笔记本上抓日志定位根因

---

## 文件 1：src/platform/win32/appbar.h

### 当前状态（有问题的代码）

```c
typedef enum {
    APP_DOCK_LEFT   = 0,  /* ABE_LEFT   */
    APP_DOCK_TOP    = 1,  /* ABE_TOP    */
    APP_DOCK_RIGHT  = 2,  /* ABE_RIGHT  */
    APP_DOCK_BOTTOM = 3   /* ABE_BOTTOM */
} AppDockEdge;

int AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness);  /* ← 4b 加的参数 */
```

### 回退后

```c
typedef enum {
    APP_DOCK_LEFT   = 0,  /* ABE_LEFT   */
    APP_DOCK_TOP    = 1,  /* ABE_TOP    */
    APP_DOCK_RIGHT  = 2,  /* ABE_RIGHT  */
    APP_DOCK_BOTTOM = 3   /* ABE_BOTTOM */
} AppDockEdge;

int AppBar_Register(HWND hwnd);  /* ← 去掉 edge/thickness 参数，回到原始签名 */
```

其他声明（Unregister、SetPosition、IsRegistered、ReRegister、ReadDockConfig）不变。

---

## 文件 2：src/platform/win32/appbar.c

### 当前状态（4b + 4d 叠加后的混乱代码）

```c
typedef struct {
    APPBARDATA data;
    int registered;
    RECT initial_workarea;    /* ← 4d 加的 */
} AppBarState;

int AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness)    /* ← 4b 改签名 */
{
    RECT rc;
    ...
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
    s_appbar.initial_workarea = rc;                                 /* ← 4d 加的 */
    ...
    s_appbar.data.uEdge = (UINT)edge;                               /* ← 4b 加的 */
    s_appbar.data.rc = rc;
    s_appbar.data.rc.left = rc.right - thickness;                   /* ← 4b 加的 */
    SHAppBarMessage(ABM_NEW, &s_appbar.data);
    ...
}

int AppBar_SetPosition(...)
{
    RECT rc = s_appbar.initial_workarea;                            /* ← 4d 改的，原始是 SPI_GETWORKAREA */
    ...
}
```

### 回退后（原始实现 + 保留 repair-2 枚举 + 2 行诊断）

```c
typedef struct {
    APPBARDATA data;
    int registered;
} AppBarState;

static AppBarState s_appbar = {0};

int AppBar_Register(HWND hwnd)
{
    if (s_appbar.registered)
        return 0;

    memset(&s_appbar.data, 0, sizeof(s_appbar.data));
    s_appbar.data.cbSize = sizeof(s_appbar.data);
    s_appbar.data.hWnd = hwnd;
    s_appbar.data.uCallbackMessage = (UINT)(WM_APP + 2);

    if (!SHAppBarMessage(ABM_NEW, &s_appbar.data))
        return 1;

    s_appbar.registered = 1;
    return 0;
}

int AppBar_Unregister(HWND hwnd)
{
    (void)hwnd;
    if (!s_appbar.registered)
        return 0;

    SHAppBarMessage(ABM_REMOVE, &s_appbar.data);
    s_appbar.registered = 0;
    return 0;
}

int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);

    /* diag: 工作区宽度 */
    {
        wchar_t buf[128];
        swprintf(buf, 128, L"[s5a-diag] workarea_w=%d thickness=%d\n",
                 rc.right - rc.left, thickness);
        OutputDebugStringW(buf);
    }

    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;

    switch (edge)
    {
    case APP_DOCK_LEFT:   s_appbar.data.rc.right = rc.left + thickness; break;
    case APP_DOCK_RIGHT:  s_appbar.data.rc.left = rc.right - thickness; break;
    case APP_DOCK_TOP:    s_appbar.data.rc.bottom = rc.top + thickness; break;
    case APP_DOCK_BOTTOM: s_appbar.data.rc.top = rc.bottom - thickness; break;
    }

    SHAppBarMessage(ABM_QUERYPOS, &s_appbar.data);
    SHAppBarMessage(ABM_SETPOS, &s_appbar.data);

    /* diag: SETPOS 后的 AppBar 矩形 */
    {
        wchar_t buf[128];
        swprintf(buf, 128, L"[s5a-diag] setpos_rc=(%d,%d,%d,%d) w=%d\n",
                 s_appbar.data.rc.left, s_appbar.data.rc.top,
                 s_appbar.data.rc.right, s_appbar.data.rc.bottom,
                 s_appbar.data.rc.right - s_appbar.data.rc.left);
        OutputDebugStringW(buf);
    }

    MoveWindow(hwnd,
               s_appbar.data.rc.left, s_appbar.data.rc.top,
               s_appbar.data.rc.right - s_appbar.data.rc.left,
               s_appbar.data.rc.bottom - s_appbar.data.rc.top, TRUE);
    return 0;
}
```

---

## 文件 3：src/platform/win32/window.c

### 当前状态（4b 改的）

```c
case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:
    if (AppBar_IsRegistered(hwnd))
        AppBar_Unregister(hwnd);
    else
    {
        AppDockEdge edge = APP_DOCK_RIGHT;
        int thickness = 240;
        AppBar_ReadDockConfig(&edge, &thickness);              /* ← 4b 加的 */
        if (AppBar_Register(hwnd, edge, thickness) == 0)       /* ← 4b 改的签名 */
            AppBar_SetPosition(hwnd, edge, thickness);
    }
    break;
```

### 回退后

```c
case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:
    if (AppBar_IsRegistered(hwnd))
        AppBar_Unregister(hwnd);
    else if (AppBar_Register(hwnd) == 0)
        AppBar_SetPosition(hwnd, APP_DOCK_RIGHT, 240);
    break;
```

---

## 文件 4：src/app/app.c

### 当前状态（4a 加的）

```c
else
{
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;

    StateData state;
    StateStore_Load(&state);
    state.dock_edge = APP_DOCK_RIGHT;

    /* 4a 动态计算块（5 行） */
    {
        RECT wa;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        state.dock_thickness = (int)((wa.right - wa.left) * 18 / 100);
        if (state.dock_thickness < 200) state.dock_thickness = 200;
        if (state.dock_thickness > 500) state.dock_thickness = 500;
    }

    StateStore_Save(&state);
}
```

### 回退后

```c
else
{
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;

    StateData state;
    StateStore_Load(&state);
    state.dock_edge = APP_DOCK_RIGHT;
    state.dock_thickness = 240;          /* 固定值 */

    StateStore_Save(&state);
}
```

---

## 执行步骤

| 步骤 | 文件 | 操作 |
|------|------|------|
| 1 | app.c | 删除 4a 的 `{ RECT wa; ... }` 代码块，恢复为 `state.dock_thickness = 240` |
| 2 | appbar.h | `AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness)` → `AppBar_Register(HWND hwnd)` |
| 3 | appbar.c | 写入上面"回退后"完整代码（原始实现 + 2 行诊断 + repair-2 枚举值） |
| 4 | window.c | ENTER_EDGE_RESERVED 分支改为原始版本 |
| 5 | 编译 | 确认无 error |
| 6 | 测试 | 开发机贴边，确认 DebugView 有 `[s5a-diag]` 输出 → 抓日志 |
