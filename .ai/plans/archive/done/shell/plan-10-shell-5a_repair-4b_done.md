# Shell-5a_repair-4b — 修复 ABM_NEW 空矩形导致幽灵保留区（后续）

## ① 核心问题

`AppBar_Register` 用 `rc={0,0,0,0}` 和 `uEdge=ABE_LEFT` 调用 `ABM_NEW`，系统用默认大小（约 300px）创建了幽灵 AppBar 保留区。之后 `ABM_SETPOS` 在正确位置注册第二个保留区。**两个保留区叠加**，导致贴边后出现约 300px 死区——DeskNote 窗口和最大化窗口之间空出一个谁也进不去的区域。

## ② 分析现状

当前流程：

```
AppBar_Register(hwnd)
  → ABM_NEW，rc={0,0,0,0}，uEdge=ABE_LEFT
  → 系统创建幽灵保留区（~300px，默认大小）

AppBar_SetPosition(hwnd, ABE_RIGHT, thickness)
  → ABM_SETPOS，rc 正确（240 或动态值）
  → 系统创建第二个保留区

结果：两个保留区叠加 ≈ 300 + 240 = 死区
```

修复方向：在 `ABM_NEW` 之前就填好正确的 `uEdge` 和 `rc`，一次注册到位。

## ③ 方案设计

只有一条合理路径：`AppBar_Register` 接受 `edge` 和 `thickness` 参数，在调用 `ABM_NEW` 前把 rc 和 uEdge 填好。同步更新 `window.c` 的调用方和 `AppBar_ReRegister`。

## ④ 拆解执行

### 整体目标

1. `AppBar_Register` 改为 `AppBar_Register(HWND, AppDockEdge, int)`
2. `window.c` 中调用时传入正确的 edge 和从 state.ini 读取的 thickness
3. `AppBar_ReRegister` 内部调用适配新签名

### 阶段产出

贴边后无死区，最大化窗口填到 DeskNote 窗口左边。

## ⑤ 设定边界

### 文件落点

| 文件 | 改动 |
|------|------|
| `appbar.h` | 函数签名改 `AppBar_Register(HWND, AppDockEdge, int)` |
| `appbar.c` | `AppBar_Register` 实现：ABM_NEW 前设置 uEdge 和 rc |
| `appbar.c` | `AppBar_ReRegister` 内部调用适配新签名 |
| `window.c` | 调用 `AppBar_Register` 时传入 thickness（从 state.ini 读） |

### 原则上不应修改

- `src/app/*`、`src/storage/*`、`src/ui/*`、`src/render/*`、`src/editor/*`

## ⑥ 落地方案

### 技术路线

**appbar.h 签名变更：**

```c
/* 注册当前窗口为 AppBar，返回 0 成功 */
int AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness);
```

**appbar.c AppBar_Register 新实现：**

```c
int AppBar_Register(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc;

    if (s_appbar.registered)
        return 0;

    /* ABM_NEW 之前就填好 uEdge 和 rc，一次到位 */
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
    memset(&s_appbar.data, 0, sizeof(s_appbar.data));
    s_appbar.data.cbSize = sizeof(s_appbar.data);
    s_appbar.data.hWnd = hwnd;
    s_appbar.data.uCallbackMessage = (UINT)(WM_APP + 2);
    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;

    /* 根据贴边方向设置初始 rc */
    if (edge == APP_DOCK_RIGHT)
        s_appbar.data.rc.left = rc.right - thickness;

    if (!SHAppBarMessage(ABM_NEW, &s_appbar.data))
        return 1;

    s_appbar.registered = 1;
    return 0;
}
```

**appbar.c AppBar_ReRegister 修复（适配新签名）：**

```c
int AppBar_ReRegister(HWND hwnd)
{
    AppDockEdge edge;
    int thickness;

    if (!s_appbar.registered)
        return 0;

    edge = (AppDockEdge)s_appbar.data.uEdge;
    thickness = s_appbar.data.rc.right - s_appbar.data.rc.left;

    s_appbar.registered = 0;
    if (AppBar_Register(hwnd, edge, thickness) != 0)
        return 1;

    return AppBar_SetPosition(hwnd, edge, thickness);
}
```

**window.c 调用方修改：**

```c
case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:
    if (AppBar_IsRegistered(hwnd))
        AppBar_Unregister(hwnd);
    else
    {
        /* 从 state.ini 读取厚度（app.c 中 cmd==103 已写入） */
        AppDockEdge edge = APP_DOCK_RIGHT;
        int thickness = 240;  /* 默认兜底 */
        AppBar_ReadDockConfig(&edge, &thickness);

        if (AppBar_Register(hwnd, edge, thickness) == 0)
            AppBar_SetPosition(hwnd, edge, thickness);
    }
    break;
```

### 主链路

```text
用户切换贴边占位
  → app.c cmd==103 → 动态计算 thickness → 存 state.ini → 提交 ENTER_EDGE_RESERVED
  → window.c 收束 → AppBar_ReadDockConfig 读 thickness
  → AppBar_Register(hwnd, edge, 276) → ABM_NEW 时 rc 和 uEdge 已正确
  → AppBar_SetPosition → ABM_SETPOS 直接确认，无需调整
  → 无幽灵保留区，死区消失
```

## ⑦ 验收标准

1. 贴边后无死区——最大化窗口右边界紧挨 DeskNote 窗口左边界
2. 退出贴边后工作区完全恢复，无残留
3. `ABM_NEW` 调用时 rc 和 uEdge 不为零
4. 编译通过

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/platform/win32/appbar.h` 的 `AppBar_Register` 签名改为 `(HWND, AppDockEdge, int)`——注册时即确定边和厚度，不再空注册 | 签名就绪 | 编译通过 |
| 2 | `src/platform/win32/appbar.c` 的 `AppBar_Register` 实现改为 ABM_NEW 前设置 uEdge 和 rc——系统初始注册就拿到正确值，不产生幽灵保留区 | 注册逻辑修复 | 编译通过 |
| 3 | `src/platform/win32/appbar.c` 的 `AppBar_ReRegister` 内部适配新签名，传入保存的 edge 和 thickness | ReRegister 兼容新签名 | 编译通过 |
| 4 | `src/platform/win32/window.c` 的 ENTER_EDGE_RESERVED 分支中，调用 `AppBar_ReadDockConfig` 读取动态厚度，传入 `AppBar_Register` 新签名——不再硬编码 240 | 厚度从 state.ini 动态读取 | 编译通过 |
| 5 | 验证：贴边后无死区 | 死区消失 | 手动验证 |
