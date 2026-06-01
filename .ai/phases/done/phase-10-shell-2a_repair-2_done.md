# Shell-2a_repair-2 — 可见性修复：菜单按钮颜色 + 文字标识（后续）

## 阶段摘要

在确认渲染通路正确后（_repair-1 已完成），修复菜单按钮的颜色对比度和文字标识，使其在 titlebar 上肉眼可见。

## 阶段目标

1. 恢复 `is_visible = 1`
2. 设 `bg_color` 为浅灰 `(210,210,210)`，与标题栏黄底 `(255,251,214)` 明显区分
3. 设 `label = L"M"`，先用简单字母排除 Unicode 字形兼容问题
4. 设 `label_color = (60,60,60)`，深灰文字在浅灰背景上清晰可读
5. 编译运行确认按钮可见

## 本阶段范围

仅修复菜单按钮的颜色参数和 label。不改功能逻辑。

## 本阶段不做

1. 不改汉堡图标（_repair-3 做）
2. 不改菜单弹出逻辑
3. 不改拖拽逻辑

## 文件落点

### 本阶段预计修改文件

- `src/ui/titlebar.c` — 恢复 `is_visible = 1`，修改颜色和 label 参数

### 本阶段原则上不应修改

- `src/ui/button.h` / `button.c`（Shell-1e 冻结）
- 所有其他文件

## 技术路线

在 `src/ui/titlebar.c` 的 `Titlebar_CalculateLayout` 中，替换菜单按钮配置为：

```c
layout.menu_button.is_visible = 1;
layout.menu_button.is_enabled = 1;
layout.menu_button.bg_color = (RenderColor){ 210, 210, 210, 255 };  /* 浅灰，与黄底明显区分 */
layout.menu_button.hover_color = (RenderColor){ 180, 180, 220, 255 };
layout.menu_button.pressed_color = (RenderColor){ 160, 160, 200, 255 };
layout.menu_button.label_color = (RenderColor){ 60, 60, 60, 255 };
layout.menu_button.label = L"M";  /* 先用 "M" 排除 Unicode 字形问题 */
```

编译运行。确认 titlebar 右上角可见一个浅灰底色的 "M" 按钮。

如果仍不可见，说明 `Titlebar_Draw` 中 `Button_Draw` 调用路径未正确接通，需检查 `Titlebar_Draw` 函数体。

## 完成标准

1. 编译通过
2. 右上角可见一个浅灰色按钮区域，中间有 "M" 文字
3. 鼠标悬停时按钮颜色变为浅蓝灰

## 计划的最后一步

1. `git add` 当前阶段修改过的文件
2. `git commit -m "Shell-2a_repair-2: fix menu button color and label"`
3. `git push`
4. 完成后重命名为 `_done` 后缀
