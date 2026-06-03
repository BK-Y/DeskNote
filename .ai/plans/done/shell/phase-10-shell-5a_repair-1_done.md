# Shell-5a_repair-1 — 修复贴边后其他最大化窗口仍侵入保留区（后续）

## ① 核心问题

Shell-5a 已完成 AppBar 注册/贴边，`ABM_SETPOS` 正确调用了，但**已经最大化状态的其他窗口不会主动重新查询工作区**——它们不知道工作区变了。导致最大化后的其他窗口仍然覆盖便签的贴边保留区域。

## ② 分析现状

### 阶段摘要

从"AppBar 注册成功但其他窗口不避让"修复为"贴边后系统工作区立即广播，所有最大化窗口自动避让"。

### 承接关系

前置阶段：Shell-5a_done（AppBar 注册/注销/贴边已就绪，但缺少工作区广播）

## ③ 方案设计

只有一条合理路径：在 `AppBar_SetPosition` 的 `ABM_SETPOS` 之后、`AppBar_Unregister` 的 `ABM_REMOVE` 之后，分别调用 `SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE)` 广播工作区变更。

`SPIF_SENDCHANGE` 会向所有顶层窗口发送 `WM_SETTINGCHANGE`，告知它们工作区已变更，强制已最大化的窗口重新查询并调整自己的大小。

## ④ 拆解执行

### 整体目标

1. `AppBar_SetPosition` 在 `ABM_SETPOS` 后广播工作区缩小，已最大化的其他窗口自动收缩避让
2. `AppBar_Unregister` 在 `ABM_REMOVE` 后广播工作区恢复，已最大化的其他窗口自动扩展

### 阶段产出

贴边后，其他已最大化窗口立即缩小，不覆盖便签保留区。

## ⑤ 设定边界

### 本阶段范围

只在 appbar.c 中加两行 `SystemParametersInfoW` 调用。

### 不包括

1. 不改任何其他文件
2. 不改 AppBar 注册/注销逻辑
3. 不做其他层改动

### 分层归属

- **platform/appbar** — 调用 `SystemParametersInfoW`（系统 API），符合分层约束

### 文件落点

#### 预计修改文件

- `src/platform/win32/appbar.c` — `AppBar_SetPosition` 和 `AppBar_Unregister` 各加一行

#### 原则上不应修改

- 所有其他文件

## ⑥ 落地方案

### 技术路线

**AppBar_SetPosition 中，`ABM_SETPOS` 之后：**

```c
/* 广播工作区变更，强制其他最大化窗口重新查询并避让 */
SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
```

**AppBar_Unregister 中，`ABM_REMOVE` 之后：**

```c
/* 恢复工作区，通知其他窗口 */
SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
```

## ⑦ 验收标准

1. 贴边后，其他已最大化窗口立即缩小避让，不覆盖便签保留区
2. 退出贴边后，其他最大化窗口恢复正常全屏
3. 编译通过，无其他文件被修改

## ⑧ 推进步骤

| 步骤 | 操作内容——设计意图 | 操作结果 | 验证方式 |
|------|--------------------|---------|---------|
| 1 | `src/platform/win32/appbar.c` 的 `AppBar_SetPosition` 中，`ABM_SETPOS` 后加 `SystemParametersInfoW(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE)`——通知系统和其他窗口工作区已调整 | 贴边后其他最大化窗口立即避让 | 编译通过 |
| 2 | `src/platform/win32/appbar.c` 的 `AppBar_Unregister` 中，`ABM_REMOVE` 后加同上广播——通知系统和其他窗口工作区已恢复 | 退出贴边后其他窗口恢复正常 | 编译通过 |
| 3 | 验证：贴边后，已最大化的其他窗口收缩不覆盖便签 | 避让正确 | 手动验证 |
| 4 | 验证：退出贴边后，其他窗口可正常全屏 | 恢复正常 | 手动验证 |
