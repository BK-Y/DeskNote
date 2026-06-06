# Plan-11c-05 — 拖拽结束

## 核心问题

App_OnEndDrag 直接调 StateStore_Save 保存模式和窗口几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| A：拖拽→浮动置顶（release_on_drag_mode=1） | `StateData state; StateStore_Load(&state); state.last_floating_* = rc.*; state.shell_resident_mode = FLOATING_TOPMOST; StateStore_Save(&state); AppBar_Unregister; g_app.shell.resident_mode = FLOATING_TOPMOST; SetWindowPos(HWND_TOPMOST)` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rc.*); Config_Set(KEY_SHELL_RESIDENT_MODE, FLOATING_TOPMOST); g_app.shell.resident_mode = FLOATING_TOPMOST; AppBar_Unregister; SetWindowPos(HWND_TOPMOST)` | 拖拽→浮动路径改用 Config_Set 持久化 mode 和几何 | 贴边→拖离边缘 → 变为浮动置顶窗口，state.ini 中 mode=1 + floating 坐标更新 |
| B：拖拽→普通窗口（release_on_drag_mode=2） | `state.shell_resident_mode = NONE; StateStore_Save(&state); AppBar_Unregister; g_app.shell.resident_mode = NONE;` → `Config_Set(KEY_SHELL_RESIDENT_MODE, NONE); g_app.shell.resident_mode = NONE; AppBar_Unregister;` | 拖拽→普通窗口路径改用 Config_Set 持久化 mode | 贴边→拖离（mode=2） → 变为普通窗口，不置顶 |

## 主链路

```
拖拽结束 → WM_EXITSIZEMOVE → App_OnEndDrag → GetWindowRect → Config_Set(last_floating_*) + Config_Set(mode) → AppBar_Unregister → SetWindowPos
```

## 验收标准

- [ ] App_OnEndDrag 中的 StateStore 调用被 Config_Set 替换
- [ ] state.ini 中 mode + 几何数据更新正确

## 分层归属

本步骤属于 app 层，将 App_OnEndDrag 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [human] 应用已启动，窗口为贴边模式（`shell_resident_mode=2`）

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 编译语法 | `gcc -fsyntax-only src/app/app.c` | 0 error |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 |
|------|---------|---------|
| M-1 | 1. 贴边→拖离边缘（拖出约 100px）<br>2. 松开鼠标 | 窗口变为浮动置顶，state.ini `shell_resident_mode=1` + floating 坐标更新 |
| M-2 | 1. 贴边→拖离边缘（拖到屏幕中央附近）<br>2. 松开鼠标 | 窗口变为普通窗口（不置顶），state.ini `shell_resident_mode=0` |

### GATE 6 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
