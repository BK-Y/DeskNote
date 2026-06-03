# Shell-3a_1 — 托盘图标创建与销毁（后续）

## ① 核心问题

窗口关闭后用户就找不到应用了。没有托盘图标作为后台入口，关了窗口等于彻底丢失应用。

## ② 分析现状

### 阶段摘要

从"启动后只有任务栏图标"推进到"启动后自动在系统托盘创建 DeskNote 图标，作为后台入口"。

### 承接关系

前置阶段：Shell-2b_done（窗口布局基线、frameless 窗口、标题栏已就绪）

## ③ 方案设计

候选方案：
1. **Win32 Shell_NotifyIcon API** — 标准做法，NOTIFYICONDATAW 结构体 + NIM_ADD/NIM_DELETE
2. **自定义窗口 + 自绘托盘** — 复杂度高，不可取

选择方案 1。理由：Win32 标准 API，简洁可靠，无需额外依赖。

## ④ 拆解执行

### 整体目标

1. 初始化 NOTIFYICONDATAW 结构体，绑定窗口句柄和自定义回调消息
2. 调用 Shell_NotifyIconW(NIM_ADD) 在托盘区域添加图标
3. 程序退出时调用 Shell_NotifyIconW(NIM_DELETE) 移除图标
4. 图标使用现有的 desknote.ico 资源

### 阶段产出

1. 程序启动后系统托盘区域出现 DeskNote 图标
2. 程序退出时托盘图标自动消失
3. 图标显示正常（使用现有 desknote.ico）

## ⑤ 设定边界

### 本阶段范围

只做托盘图标的创建和销毁。

### 不包括

1. 不做托盘菜单弹出
2. 不做左键/右键点击响应
3. 不做隐藏到托盘逻辑
4. 不改窗口显示/隐藏行为

### 分层归属

- **platform/win32/window** — Window_Run 中调用托盘初始化，WndProc 注册 WM_APP+1 回调，WM_DESTROY 中销毁
- **app/** — app.h 声明接口，app.c 中 App_Init/App_Shutdown 调用
- **ui/、render/、editor/、storage/、core/** — 不参与

### 文件落点

#### 预计修改文件

- `src/platform/win32/window.c`
- `src/app/app.h`
- `src/app/app.c`

#### 原则上不应修改

- `src/ui/*`、`src/render/*`、`src/editor/*`、`src/core/*`、`src/storage/*`

## ⑥ 落地方案

### 技术路线

**app.h** 新增声明：

```c
int App_InitTrayIcon(HWND hwnd);
void App_DestroyTrayIcon(HWND hwnd);
```

**app.c** 实现：

```c
#define WM_TRAYICON (WM_APP + 1)

int App_InitTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    HICON hIcon;

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    wcsncpy(nid.szTip, L"DeskNote", sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);

    hIcon = LoadIconW(GetModuleHandleW(NULL), L"IDI_DESKNOTE");
    if (hIcon == NULL)
        hIcon = LoadIconW(NULL, IDI_APPLICATION);
    nid.hIcon = hIcon;

    return Shell_NotifyIconW(NIM_ADD, &nid) ? 0 : 1;
}

void App_DestroyTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}
```

**window.c** 在 Window_Run 中，App_Init 之后加入：

```c
if (App_Init(hwnd) != 0)
{
    DestroyWindow(hwnd);
    return 1;
}

/* Shell-3a_1: 初始化托盘图标 */
(void)App_InitTrayIcon(hwnd);

ShowWindow(hwnd, SW_SHOW);
```

在 WM_DESTROY 中 App_Shutdown 之前加入：

```c
case WM_DESTROY:
    KillTimer(hwnd, WINDOW_AUTOSAVE_TIMER_ID);
    App_DestroyTrayIcon(hwnd);
    App_Shutdown();
    Platform_Nonclient_Shutdown(hwnd);
    PostQuitMessage(0);
    return 0;
```

### 主链路

```text
程序启动
-> Window_Run 创建窗口
-> App_Init 初始化应用
-> App_InitTrayIcon → LoadIcon → Shell_NotifyIcon(NIM_ADD)
-> 托盘区域出现 DeskNote 图标（无需任何额外操作，启动即有）

程序退出
-> WM_DESTROY → App_DestroyTrayIcon → Shell_NotifyIcon(NIM_DELETE)
-> 托盘图标消失
```

### 接口说明

```c
// 初始化托盘图标，返回 0 成功，非 0 失败
int App_InitTrayIcon(HWND hwnd);

// 销毁托盘图标
void App_DestroyTrayIcon(HWND hwnd);
```

调用顺序：App_Init → App_InitTrayIcon → ... → App_DestroyTrayIcon → App_Shutdown

## ⑦ 验收标准

1. 编译通过
2. 启动后系统托盘区域出现 DeskNote 图标
3. 关闭程序后托盘图标自动消失
4. 图标使用应用自带的图标文件（非默认图标）
5. 分层归属与文件落点一致

## ⑧ 推进步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | `src/app/app.h` 新增 `App_InitTrayIcon(HWND)` 和 `App_DestroyTrayIcon(HWND)` 声明 | app.h 新增两个函数声明 | 编译通过 |
| 2 | `src/app/app.c` 新增 `WM_TRAYICON` 宏定义 + 两个函数实现 | 托盘初始化/销毁函数可调用 | 编译通过 |
| 3 | `src/platform/win32/window.c` 的 `Window_Run` 中，App_Init 后插入 `App_InitTrayIcon(hwnd)` 调用 | 启动时自动创建托盘图标 | 编译通过，启动后托盘出现图标 |
| 4 | `src/platform/win32/window.c` 的 `WM_DESTROY` 中，App_Shutdown 前插入 `App_DestroyTrayIcon(hwnd)` 调用 | 退出时自动销毁托盘图标 | 关闭程序后托盘图标消失 |
| 5 | 验证图标资源 ID 可用（确认 `IDI_DESKNOTE` 在 `.rc` 文件中存在），如果不存在则新增资源定义 | 图标正常显示 | 启动后托盘图标不是默认白框 |
