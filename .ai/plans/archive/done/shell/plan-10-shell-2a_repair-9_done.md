# Shell-2a_repair-9 — 恢复菜单按钮 hover 色为暖黄色（后续）

## 阶段摘要

将验证阶段使用的亮红色 MENU_BTN_HOVER 恢复为与标题栏协调的暖黄色。

## 阶段目标

1. MENU_BTN_HOVER 从 (255,80,80) 恢复为 (240,225,180)
2. 确认 hover 状态驱动链路正常，颜色差异肉眼可辨

## 本阶段范围

只改 titlebar.c 中一个颜色常量。

## 文件落点

### 本阶段预计修改文件

- src/ui/titlebar.c

## 技术路线

```c
static const RenderColor MENU_BTN_HOVER   = { 240, 225, 180, 255 };  /* 暖黄 hover */
```

## 完成标准

1. 编译通过
2. 鼠标移到 ☰ 按钮时颜色从 (242,238,210) 变为 (240,225,180)
3. 鼠标移出窗口时颜色恢复为 (242,238,210)

## 计划的最后一步

1. git add + commit + push + _done
