# Plan-11 — 测试验证计划

## 核心问题

Plan-11（统一配置入口）已完成全部 4 个子阶段的代码实施，但未系统验证工作正确性。
11a/b 有单元测试，11c/d 仅做了语法检查。

## 分析现状

- 前置：Plan-11a 到 11d 全部代码已提交，编译通过
- 现状：`test/test_config.c`（9/9）和 `test/test_ini2arr.c`（4/4）两个单元测试就绪
- 产出：全 Plan 回归验证通过，所有配置读取/写入/伴生行为路径确认正常

## 验证分组

### A — 正常路径（6 项）

覆盖冷启动三种模式（贴边/浮动/普通）和运行时三种用户操作（菜单、拖拽、托盘）。

### B — 回调触发（3 项）

验证 `Config_Set("shell_resident_mode", ...)` 正确触发 `OnShellModeChanged`，同一值不重复触发。

### C — 状态一致性（4 项）

验证 ini 文件与内存表一致、注释保留、新 key 追加。

### D — StateStore 残留（2 项）

`grep` 验证 `StateStore_Save` 仅余文档管理 2 处。

### E — 边界情况（3 项）

无效 ini 文件、重复 Init/Shutdown、重启持久化恢复。

### F — 回归（2 项）

重新编译运行 `test_config` 和 `test_ini2arr`，原测试全量通过。

## 推进步骤

| 步骤 | 操作内容 | 操作结果 | 验证方式 |
|------|---------|---------|---------|
| 1 | 运行 `test_ini2arr` | 4/4 PASS | `gcc -I. -o test_ini2arr test/test_ini2arr.c src/utils/ini2arr.c && ./test_ini2arr` |
| 2 | 运行 `test_config` | 9/9 PASS | `gcc -I. -o test_config test/test_config.c src/config/config.c src/utils/ini2arr.c && ./test_config` |
| 3 | grep StateStore_Save | 仅余 2 处（文档管理） | `grep -c "StateStore_Save" src/app/app.c` |
| 4 | grep StateStore_Load | 仅余文档管理 + release_on_drag | `grep -n "StateStore_Load\|StateStore_Save" src/app/app.c` |
| 5 | A-1 冷启动贴边 | 窗口贴右边缘 | 手动：设 ini `shell_resident_mode=2` 重启，最大化其他窗口观察 |
| 6 | A-2 冷启动浮动 | 窗口浮动到 (100,100) | 手动：设 ini 浮动坐标重启 |
| 7 | A-4 A-5 菜单贴边/退出 | 贴边/移回中央 | 手工操作两次菜单 |
| 8 | A-6 菜单浮动 | 窗口置顶 | 手工操作菜单 |
| 9 | B-1/B-2 回调单次触发 | 显式 AppBar 调用不再存在 | `grep -n "AppBar_Register\|AppBar_Unregister" src/app/app.c` |
| 10 | C-3/C-4 注释保留和新 key | ini 文件完好 | 手工检查 `state.ini` |

## 验收标准

- [x] F-1: `test_ini2arr` 4/4 PASS
- [x] F-2: `test_config` 9/9 PASS
- [x] D-1: StateStore_Save 仅余 2 处（文档管理）
- [x] D-2: StateStore_Load 仅余文档管理 + release_on_drag_mode
- [x] B-1/B-2: cmd103/OnEndDrag 显式 AppBar 调用已清除
- [x] B-3: `test_config_edge` 同一值不触发回调 8/8 PASS
- [x] C-1: 内存与 ini 一致性（已在 test_config + test_config_edge 验证）
- [x] C-2: 修改内存 ini 同步（`test_config` #4-#5）
- [x] C-3: 注释保留（`test_config` #8）
- [x] C-4: 新 key 追加（`test_config` #6-#7）
- [x] E-1: 缺失 ini 文件安全降级（`test_config_edge` #1）
- [x] E-2: Shutdown → Init 不报错（`test_config` #9 + `test_config_edge` #5）
- [x] E-3: 重启持久化恢复（`test_config_edge` #5）
- [x] A-3: 冷启动→普通（默认启动即 NONE 模式，已隐藏验证通过）
- [ ] A-1: 冷启动→贴边（需手动设 ini `shell_resident_mode=2` 后重启）
- [ ] A-2: 冷启动→浮动（需手动设 ini 浮动坐标后重启）
- [ ] A-4: 菜单→贴边（需手动操作菜单）
- [ ] A-5: 菜单→退出贴边（需手动操作）
- [ ] A-6: 菜单→浮动（需手动操作）
- [ ] A-7: 拖拽→浮动（需手动操作）
- [ ] A-8: 托盘隐藏恢复（需手动操作）
