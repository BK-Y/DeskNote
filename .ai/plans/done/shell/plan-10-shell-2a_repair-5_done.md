# Shell-2a_repair-5 — button 改用 Render_DrawTextCentered 居中绘制（后续）

## 阶段摘要

将 button.c 中的 Render_DrawText 替换为 Render_DrawTextCentered，使按钮 label 在其区域内水平垂直居中。

## 阶段目标

1. Button_Draw 中 label 绘制改为调用 Render_DrawTextCentered
2. 确保 label==NULL 时不调用居中绘制（行为不变）

## 本阶段范围

只改 button.c。不改 render/、titlebar/、app/ 任何文件。

## 文件落点

### 本阶段预计修改文件

- src/ui/button.c

### 本阶段原则上不应修改

- 所有其他文件

## 技术路线

将 Button_Draw 中的渲染逻辑替换为：

```c
if (btn->label != NULL) {
    text_color = btn->label_color;
    if (text_color.r == 0 && text_color.g == 0 && text_color.b == 0)
        text_color = (RenderColor){ 40, 40, 40, 255 };
    Render_DrawTextCentered(ctx, btn->label, btn->rect, text_color);
}
```

## 完成标准

1. 编译通过
2. 菜单按钮 ☰ 在按钮区域内水平垂直居中
3. 无 label 的按钮不受影响

## 计划的最后一步

1. git add + commit -m "Shell-2a_repair-5: button label uses Render_DrawTextCentered"
2. git push
3. 完成后重命名为 _done 后缀
