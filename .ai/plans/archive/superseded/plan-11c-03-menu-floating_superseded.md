# Plan-11c-03 — 菜单浮动置顶

## 核心问题

菜单 cmd102 直接调 StateStore_Load/Save 保存窗口几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| cmd102 保存浮动几何 | `StateData state; StateStore_Load(&state); state.last_floating_* = rect.*; StateStore_Save(&state);` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rect.*)` | cmd102 浮动置顶路径改用 Config_Set 持久化窗口几何 | 移动窗口位置 → 菜单→浮动置顶 → 再点取消 → 窗口回到之前记录的坐标 |

## 主链路

```
菜单→浮动置顶 → cmd102 → GetWindowRect → Config_Set(last_floating_*) → SetWindowPos(HWND_TOPMOST)
```

## 验收标准

- [ ] cmd102 中的 StateStore 调用被 Config_Set 替换
- [ ] 窗口几何在切浮动模式后被持久化到 state.ini

## 分层归属

本步骤属于 app 层，将 cmd102 handler 中的 StateStore_Save 改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [human] 应用已启动，窗口在普通模式

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| A-1 | 编译语法 | `gcc -fsyntax-only src/app/app.c` | 0 error |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 |
|------|---------|---------|
| M-1 | 1. 移动窗口到屏幕左侧<br>2. 菜单→浮动置顶 | 窗口置顶（HWND_TOPMOST），state.ini `shell_resident_mode=1` |
| M-2 | 1. 再次菜单→浮动置顶（取消）<br>2. 观察窗口 | 取消置顶，state.ini `shell_resident_mode=0` |

### GATE 5 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
