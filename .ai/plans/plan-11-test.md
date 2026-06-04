# Plan-11 — 测试验证计划

## 核心问题

Plan-11 共 4 个子阶段（11a~11d），每个子阶段完成后应有对应测试计划。
测试文件行末加 `// test - plan 11x` 标记，便于追溯和清理。

---

## 前置模块测试（非 Plan-11 自身，但被依赖）

### ini2arr（`test/test_ini2arr.c`）

```
// test - plan 11b dep (ini2arr module)
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| DEP-1 | 读 ini 文件 2 条 key=value | 返回 2 条 | ✅ 4/4 PASS |
| DEP-2 | 修改已有 key | 文件值更新，注释保留 | ✅ |
| DEP-3 | 追加新 key | 文件末尾追加 | ✅ |

---

## 11a — 模块骨架

### 无专项测试

11a 只创建空文件骨架，由 11b 的测试覆盖。

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11a-1 | `gcc -fsyntax-only src/config/config.c` | 0 error | ✅ |

---

## 11b — ini2arr 接入（`test/test_config.c`）

```
/* test - plan 11b (Config_Init/Get/Set/Shutdown full suite) */
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11b-1 | Config_Init → 读 ini | 返回 0 | ✅ |
| 11b-2 | Config_Get 已有 key | 返回匹配值 | ✅ |
| 11b-3 | Config_Get 不存在的 key | 返回 default_val | ✅ |
| 11b-4 | Config_Set 改已有 key | 返回 0 | ✅ |
| 11b-5 | Config_Get 确认改后值 | 返回新值 | ✅ |
| 11b-6 | Config_Set 新 key | 内存表追加 | ✅ |
| 11b-7 | Config_Get 新 key | 返回新值 | ✅ |
| 11b-8 | 检查 ini 文件 | 注释保留 | ✅ |
| 11b-9 | Config_Shutdown → 再 Config_Set | 返回错误（已释放） | ✅ |
| 11b-10 | `test/test_config_edge.c` E-1：缺失 ini | Config_Init 返回错误 | ✅ |
| 11b-11 | `test/test_config_edge.c` E-3：Shutdown→Init→重新读取 | 值持久化 | ✅ |

---

## 11c — 迁移写入点

### 11c-01 启动恢复

```
// test - plan 11c-01
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11c-01-1 | syntax check `gcc -fsyntax-only src/app/app.c` | 0 error | ✅ |
| 11c-01-2 | grep StateStore_Save | 仅余文档管理 | ✅ |
| 11c-01-3 | A-1 冷启动→贴边（`state.ini` `shell_resident_mode=2`） | 窗口贴右边缘 | ✅ 已手动验证 |
| 11c-01-4 | A-2 冷启动→浮动（`shell_resident_mode=1` + floating 坐标） | 窗口浮动至坐标位 | ❌ 找不到窗口 |
| 11c-01-5 | A-3 冷启动→普通（`shell_resident_mode=0`） | 普通窗口 | ✅ |
| 11c-01-6 | D: titlebar_height / frame_visual_thickness 从 Config 读 | 编译通过 + 启动后生效 | ✅ syntax |

### 11c-02 菜单贴边

```
// test - plan 11c-02
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11c-02-1 | syntax check | 0 error | ✅ |
| 11c-02-2 | A-4 菜单→贴边 | 窗口贴右边缘，ini 写入 dock 值 | ❌ 需手动 |
| 11c-02-3 | A-5 菜单→退出贴边 | 窗口移回中央 | ❌ 需手动 |

### 11c-03 菜单浮动

```
// test - plan 11c-03
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11c-03-1 | syntax check | 0 error | ✅ |
| 11c-03-2 | A-6 菜单→浮动置顶 | 窗口置顶 | ❌ 需手动 |

### 11c-04 cmd103 保存几何

```
// test - plan 11c-04
```
由 11c-02 + 11c-05 覆盖，无独立测试。

### 11c-05 拖拽结束

```
// test - plan 11c-05
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11c-05-1 | syntax check | 0 error | ✅ |
| 11c-05-2 | A-7 贴边→拖离→浮动 | 浮动置顶，ini 写入几何 | ❌ 需手动 |

### 11c-06 托盘

```
// test - plan 11c-06
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11c-06-1 | syntax check | 0 error | ✅ |
| 11c-06-2 | A-8 贴边→隐藏→恢复 | 恢复后正确状态 | ❌ 需手动 |

---

## 11d — 回调注册与收尾（`test/test_config_edge.c`）

```
/* test - plan 11d (callback same-value guard, callback count) */
```

| 编号 | 操作 | 预期 | 结果 |
|------|------|------|------|
| 11d-1 | 相同值 Config_Set | 不触发 callback | ✅ |
| 11d-2 | 不同值 Config_Set | 触发 callback 一次 | ✅ |
| 11d-3 | 连续不同值 Config_Set | callback 计数递增 | ✅ |
| 11d-4 | cmd103/OnEndDrag 显式 AppBar 调用已清除 | grep 确认 | ✅ |
| 11d-5 | App_EnableCustomChrome 不再调 StateStore | grep 确认 | ✅ |

---

## 总验收

| 分组 | 通过 | 待办 |
|------|------|------|
| DEP ini2arr | 4/4 | — |
| 11a 骨架 | 1/1 | — |
| 11b ini2arr 接入 | 11/11 | — |
| 11c-01 启动恢复 | 5/6 | A-2 冷启动浮动（找不到窗口） |
| 11c-02 菜单贴边 | 1/3 | A-4 A-5 需手动 |
| 11c-03 菜单浮动 | 1/2 | A-6 需手动 |
| 11c-05 拖拽 | 1/2 | A-7 需手动 |
| 11c-06 托盘 | 1/2 | A-8 需手动 |
| 11d 回调 | 5/5 | — |
