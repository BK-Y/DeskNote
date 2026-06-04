# Plan-11c-04 — cmd103 进入贴边前保存浮动几何

## 目标
将 cmd103 中保存浮动几何的 StateStore_Save 替换为 Config_Set。

## 前置
Config_Init 已调用。

## 步骤

原代码（app.c L1435-1450）：

```c
StateData st; StateStore_Load(&st);
st.last_floating_left   = rect.left;
st.last_floating_top    = rect.top;
st.last_floating_width  = rect.right - rect.left;
st.last_floating_height = rect.bottom - rect.top;
StateStore_Save(&st);
```

改为：

```c
Config_Set(KEY_FLOATING_LEFT, rect.left);
Config_Set(KEY_FLOATING_TOP, rect.top);
Config_Set(KEY_FLOATING_WIDTH, rect.right - rect.left);
Config_Set(KEY_FLOATING_HEIGHT, rect.bottom - rect.top);
```

## 验证

窗口浮动在某位置 → 切贴边 → 再退出贴边 → 窗口应能恢复到贴边前的位置。

## 分层归属

本步骤属于 app 层，将 cmd103 浮动几何保存部分的 StateStore 调用改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*
