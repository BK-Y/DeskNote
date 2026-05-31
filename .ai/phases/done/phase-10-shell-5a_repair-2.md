# Shell-5a_repair-2 — 修复 AppDockEdge 枚举值与 Windows ABE 常量不匹配（后续）

## ① 核心问题

`AppDockEdge` 枚举值与 Windows SDK 的 `ABE_LEFT/TOP/RIGHT/BOTTOM` 常量不匹配——`APP_DOCK_RIGHT=1` 被 Windows 当成 `ABE_TOP`，`APP_DOCK_TOP=2` 被当成 `ABE_RIGHT`。导致 `ABM_SETPOS` 在错误的方向上创建工作区保留，最大化窗口不会在右边避让。

同时，repair-1 中错误添加的 `SPI_SETWORKAREA(NULL, SPIF_SENDCHANGE)` 两行破坏了系统任务栏的 AppBar 保留区，需一并回退。

## ② 分析现状

### 枚举值对照

| Windows 常量 | 值 | AppDockEdge | 值 | 匹配？ |
|-------------|-----|------------|-----|--------|
| `ABE_LEFT` | 0 | `APP_DOCK_LEFT` | 0 | ✅ |
| `ABE_TOP` | 1 | `APP_DOCK_RIGHT` | 1 | ❌ |
| `ABE_RIGHT` | 2 | `APP_DOCK_TOP` | 2 | ❌ |
| `ABE_BOTTOM` | 3 | `APP_DOCK_BOTTOM` | 3 | ✅ |

### 连带问题

repair-1 中加入的 `SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE)` 用 NULL 覆盖工作区，破坏了系统任务栏的保留区。

## ③ 方案设计

只有一条合理路径：
1. 将 `AppDockEdge` 枚举值改为与 `ABE_*` 一致，并调整顺序为 `LEFT / TOP / RIGHT / BOTTOM`
2. 回退 repair-1 的两行 `SPI_SETWORKAREA(NULL, SPIF_SENDCHANGE)` 调用

## ④ 拆解执行

### 整体目标

1. `AppDockEdge` 枚举值对齐 Windows ABE_* 常量
2. 移除 repair-1 中破坏任务栏的 `SPI_SETWORKAREA` 调用
3. `ABM_SETPOS` 正确在右侧创建保留区，最大化窗口自动避让

### 阶段产出

贴边后，其他最大化窗口在右侧自动避让。

## ⑤ 设定边界

### 本阶段范围

只改 `appbar.h` 枚举值和 `appbar.c` 中的两行调用。

### 不包括

1. 不改 AppBar 注册/注销逻辑
2. 不改其他文件和 API 调用

### 分层归属

- **platform/appbar.h** — 枚举值修正
- **platform/appbar.c** — 移除 SPI_SETWORKAREA 调用

### 文件落点

#### 预计修改文件

- `src/platform/win32/appbar.h` — 枚举值修正
- `src/platform/win32/appbar.c` — 删除 repair-1 的两行

#### 原则上不应修改

- 所有其他文件

## ⑥ 落地方案

### 技术路线

**appbar.h 枚举修正：**

```c
/* AppBar 贴边方向（必须与 Windows ABE_* 常量值一致） */
typedef enum {
    APP_DOCK_LEFT   = 0,  /* ABE_LEFT   */
    APP_DOCK_TOP    = 1,  /* ABE_TOP    */
    APP_DOCK_RIGHT  = 2,  /* ABE_RIGHT  */
    APP_DOCK_BOTTOM = 3   /* ABE_BOTTOM */
} AppDockEdge;
```

**appbar.c 回退 repair-1：**

删除 `AppBar_SetPosition` 中 `ABM_SETPOS` 后的：
```c
SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
```

删除 `AppBar_Unregister` 中 `ABM_REMOVE` 后的同款调用。

## ⑦ 验收标准

1. 贴边后，`ABM_SETPOS` 在右侧正确创建保留区
2. 最大化其他窗口时自动避让便签右侧区域
3. 系统任务栏正常可点击
4. 编译通过

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/platform/win32/appbar.h` 的 `AppDockEdge` 枚举改为 `LEFT=0, TOP=1, RIGHT=2, BOTTOM=3`，与 `ABE_*` 一致——ABM_SETPOS 在正确方向创建保留区 | 枚举值修复 | 编译通过 |
| 2 | `src/platform/win32/appbar.c` 的 `AppBar_SetPosition` 中删除 `SPI_SETWORKAREA(NULL, SPIF_SENDCHANGE)` 行——回退 repair-1 的破坏性调用 | 不再破坏任务栏 | 编译通过 |
| 3 | `src/platform/win32/appbar.c` 的 `AppBar_Unregister` 中删除同款调用——同上 | 退出贴边时不破坏任务栏 | 编译通过 |
| 4 | 验证：贴边后最大化其他窗口自动避让 | 避让正确 | 手动验证 |
| 5 | 验证：系统任务栏正常可点击 | 任务栏正常 | 手动验证 |
