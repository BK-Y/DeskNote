# Plan-11c-04 — cmd103 进入贴边前保存浮动几何

## 核心问题

cmd103 从浮动切贴边前，直接调 StateStore_Save 保存浮动几何，不走 Config。

## 前置

Config_Init 已调用。

## 方案选型

只有一条可行路径：用 Config_Get/Set 直接替换 StateStore 读/写。不引入中间层，不 cache 到新结构。

## 步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| cmd103 保存浮动几何 | `StateData st; StateStore_Load(&st); st.last_floating_* = rect.*; StateStore_Save(&st);` → `Config_Set(KEY_FLOATING_LEFT/TOP/WIDTH/HEIGHT, rect.*)` | cmd103 从浮动切贴边前改用 Config_Set 保存浮动几何 | 窗口浮动在某位置 → 切贴边 → 再退出贴边 → 窗口应能恢复到贴边前的位置 |

## 主链路

```
菜单→从浮动切贴边 → cmd103 → GetWindowRect → Config_Set(last_floating_*) → Config_Set(dock) → AppBar ops
```

## 验收标准

- [ ] cmd103 浮动几何保存中的 StateStore_Save 被 Config_Set 替换
- [ ] 切贴边后用 Config_Get 读到刚保存的浮动几何

## 分层归属

本步骤属于 app 层，将 cmd103 浮动几何保存部分的 StateStore 调用改为 Config_Set。

## 不修改

src/render/*、src/editor/*、src/core/*、src/ui/*、src/platform/*、src/storage/*

## 测试用例

### 前置条件
- [agent] 构建产物 `build/desknote.exe` 已生成
- [human] 确保旧进程已关闭
- [human] 应用已启动，窗口在浮动置顶模式（`shell_resident_mode=1`），有有效的浮动坐标
- 本子阶段无独立测试，由 GATE 4 + GATE 6 交叉覆盖

### 自动化检查  [agent 执行]
| 编号 | 验证内容 | 命令 | 预期结果 |
|------|---------|------|---------|
| — | 本子阶段无独立自动化检查 | — | — |

### 手工验证  [human 执行]
| 编号 | 操作步骤 | 预期结果 | 交叉验证来源 |
|------|---------|---------|-------------|
| M-1 | 1. 窗口浮动在 (100,200)<br>2. 菜单→贴边占位<br>3. 菜单→退出贴边 | 窗口恢复到 (100,200) 位置 | 由 11c-02 M-1 / M-2 间接验证 |
| M-2 | 检查组: `grep -E 'last_floating' state.ini` | floating 坐标与操作前一致 | 交叉验证 11c-02 + 11c-05 |

### GATE 4+6 通过条件
- [ ] [agent] 全部自动化检查通过
- [ ] [human] 全部手工验证通过，结果已反馈
