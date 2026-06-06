# Shell-5a_repair-4d — 修复 ABM_NEW 后 SetPosition 重复读工作区导致双倍扣除（后续）

## ① 核心问题

`AppBar_SetPosition` 中再次调用 `SPI_GETWORKAREA` 读工作区宽度。如果当前 Windows 版本在 `ABM_NEW` 后立即调整了工作区，SetPosition 读到的宽度已是调整后的值，在此基础上再扣 thickness 会导致保留区翻倍。

```
Register → ABM_NEW
  ├─ 开发机：不调整工作区，见 ①
  └─ 笔记本：立即调整工作区，见 ②

SetPosition → SPI_GETWORKAREA ① → 读到原始宽度（如 1920）
            → rc.left = 1920 - 280 = 1640 → 正确

SetPosition → SPI_GETWORKAREA ② → 读到调整后宽度（如 1640）
            → rc.left = 1640 - 280 = 1360 → 双倍扣除 ❌
```

## ② 分析现状

`AppBar_Register`（4b 已修复）在 `ABM_NEW` 前已调用过一次 `SPI_GETWORKAREA` 并填好了 rc。这次调用得到的原始工作区可保存下来供 SetPosition 复用，无需再读。

## ③ 方案设计

只有一条合理路径：`AppBar_Register` 中将初始工作区 RECT 保存到 `s_appbar` 中，`AppBar_SetPosition` 使用这个保存的值替代再次调用 `SPI_GETWORKAREA`。

## ④ 拆解执行

### 整体目标

1. `AppBarState` 新增 `RECT initial_workarea` 字段
2. `Register` 中 `SPI_GETWORKAREA` 后保存到该字段
3. `SetPosition` 中改用 `s_appbar.initial_workarea` 替代重新 `SPI_GETWORKAREA`

### 阶段产出

不同 Windows 版本下贴边厚度始终为单次扣除，无双倍死区。

## ⑤ 设定边界

### 文件落点

- `src/platform/win32/appbar.c` — 状态结构体 + Register 保存 + SetPosition 引用

### 原则上不应修改

- `src/app/*`、`src/storage/*`、`src/ui/*`、`src/platform/win32/window.c`

## ⑥ 落地方案

### 技术路线

**appbar.c 结构体新增字段：**

```c
typedef struct {
    APPBARDATA data;
    int registered;
    RECT initial_workarea;  /* Register 时保存的初始工作区，供 SetPosition 使用 */
} AppBarState;
```

**AppBar_Register 中保存初始工作区：**

```c
SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
s_appbar.initial_workarea = rc;  /* ← 保存 */

s_appbar.data.uEdge = (UINT)edge;
s_appbar.data.rc = rc;
/* ... */
```

**AppBar_SetPosition 改用保存的值：**

```c
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness)
{
    RECT rc = s_appbar.initial_workarea;  /* ← 改用保存的值，不再 SPI_GETWORKAREA */

    s_appbar.data.uEdge = (UINT)edge;
    s_appbar.data.rc = rc;
    /* ... 后续不变 */
```

## ⑦ 验收标准

1. ABM_NEW 后 SetPosition 不重新读工作区，固定使用 Register 时的原始值
2. 开发机和笔记本上死区一致消失
3. 编译通过

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `appbar.c` 的 `AppBarState` 新增 `RECT initial_workarea`——保存初始工作区 | 结构体就绪 | 编译通过 |
| 2 | `AppBar_Register` 中 `SPI_GETWORKAREA` 后 `s_appbar.initial_workarea = rc`——保存原始值供 SetPosition 复用 | 保存原始工作区 | 编译通过 |
| 3 | `AppBar_SetPosition` 中 `RECT rc = s_appbar.initial_workarea` 替换 `SPI_GETWORKAREA`——不再重复读取 | 不再重复扣除 | 编译通过 |
| 4 | 删除诊断代码（`<wchar.h>`、`<debugapi.h>`、两个 `OutputDebugStringW` 块）——清理临时诊断 | 代码恢复干净 | 编译通过 |
