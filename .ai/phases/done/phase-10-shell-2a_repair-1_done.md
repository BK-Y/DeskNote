# Shell-2a_repair-1 — 隔离验证：隐藏菜单按钮，确认组件通道（后续）

## 阶段摘要

通过隐藏菜单按钮来验证按钮组件是否正确接入了 titlebar 渲染管线。

## 阶段目标

1. 设 `menu_button.is_visible = 0`
2. 编译运行，观察"右上角长条区域"是否消失
3. 记录验证结果：消失 → 渲染通路正常，根因在视觉参数；未消失 → 接线未正确接入

## 本阶段范围

仅设 `is_visible = 0` 做隔离。不做任何视觉修复。

## 本阶段不做

1. 不改颜色参数
2. 不改 label
3. 不改 button.h/c

## 文件落点

### 本阶段预计修改文件

- `src/ui/titlebar.c` — `Titlebar_CalculateLayout` 末尾加一行 `layout.menu_button.is_visible = 0;`

### 本阶段原则上不应修改

- 所有其他文件

## 技术路线

在 `src/ui/titlebar.c` 的 `Titlebar_CalculateLayout` 函数中，在 `layout.menu_button.is_enabled = 1;` 之后追加：

```c
layout.menu_button.is_visible = 0;  /* Shell-2a_repair-1：隔离验证 */
```

编译运行，观察右上角标题栏区域。记录结果。

## 完成标准

1. 编译通过
2. 观察到右上角"长条区域"消失

## 计划的最后一步

1. `git add` 当前阶段修改过的文件
2. `git commit -m "Shell-2a_repair-1: hide menu button for isolation verification"`
3. `git push`
4. 完成后重命名为 `_done` 后缀
