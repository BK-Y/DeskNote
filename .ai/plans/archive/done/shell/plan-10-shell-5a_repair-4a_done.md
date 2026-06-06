# Shell-5a_repair-4a — 核心自适应：工作区宽度比例计算（后续）

## ① 核心问题

`dock_thickness = 240` 是写死值，在不同分辨率下体验差异大：
- 1536×864 屏：240px = 15.6%，偏窄
- 1920×1080 屏：240px = 12.5%，明显窄
- 1366×768 屏：240px = 17.6%，还行

## ② 分析现状

`app.c` 的 `cmd == 103` 写死 `state.dock_thickness = 240`。`AppBar_SetPosition` 已接受动态参数，无需改动。

## ③ 方案设计

只有一条合理路径：`SPI_GETWORKAREA` 获取工作区宽度，按 18% 计算，clamp [200, 500]。

## ④ 拆解执行

### 整体目标

替换 `state.dock_thickness = 240` 为动态计算。

### 阶段产出

首次进入贴边时厚度按屏幕宽度自适应。

## ⑤ 设定边界

### 分层归属

- **app/** — 菜单命令处理中动态计算

### 文件落点

- `src/app/app.c` — `cmd == 103` 分支

## ⑥ 落地方案

```c
/* Shell-5a-repair-4a: 动态计算默认厚度 */
RECT wa;
SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
state.dock_thickness = (int)((wa.right - wa.left) * 18 / 100);
if (state.dock_thickness < 200) state.dock_thickness = 200;
if (state.dock_thickness > 500) state.dock_thickness = 500;
```

## ⑦ 验收标准

| 工作区宽度 | 期望厚度 |
|-----------|---------|
| 1366 | 245 |
| 1536 | 276 |
| 1920 | 345 |
| 2560 | 460 |

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/app/app.c` 的 `cmd == 103` 中 `state.dock_thickness = 240` 改为动态计算——厚度按屏幕宽度比例自适应 | 厚度动态计算 | 编译通过 |
