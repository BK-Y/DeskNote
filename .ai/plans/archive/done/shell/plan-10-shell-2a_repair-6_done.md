# Shell-2a_repair-6 — titlebar 菜单按钮配色调整（后续）

## 阶段摘要

将菜单按钮颜色从浅灰改为黄底同色系，使其与标题栏背景视觉协调。

## 阶段目标

1. MENU_BTN_BG 改为 (242,238,210)，与标题栏 (255,251,214) 同色系
2. MENU_BTN_HOVER 改为 (240,225,180)，暖黄色悬停反馈
3. MENU_BTN_PRESSED 改为 (230,210,160)，按下态加深
4. label_color 改为 (140,120,80)，深棕与黄底搭配

## 本阶段范围

只改 titlebar.c 中的颜色常量。不改 button/、render/、app/ 任何文件。

## 文件落点

### 本阶段预计修改文件

- src/ui/titlebar.c

### 本阶段原则上不应修改

- 所有其他文件

## 技术路线

src/ui/titlebar.c 中修改三个常量 + 一处 label_color 赋值：

```c
static const RenderColor MENU_BTN_BG      = { 242, 238, 210, 255 };
static const RenderColor MENU_BTN_HOVER   = { 240, 225, 180, 255 };
static const RenderColor MENU_BTN_PRESSED = { 230, 210, 160, 255 };
```

```c
layout.menu_button.label_color = (RenderColor){ 140, 120, 80, 255 };
```

## 完成标准

1. 编译通过
2. 菜单按钮底色为暖黄，与标题栏黄底协调而非突兀
3. 悬停时变为更暖的黄色，肉眼可辨
4. 按下时进一步加深

## 计划的最后一步

1. git add + commit + push
2. 完成后重命名为 _done 后缀
