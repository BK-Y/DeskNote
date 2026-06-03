# 保存安全主线核心文档

## 用途

本文件只定义 `Phase 6-9` 共用的：

1. 保存安全内存模型
2. 关键工作流程
3. 各模块在流程中的职责
4. 四个阶段各自负责补哪一层能力

## 保存安全内存模型

后续 `Phase 6-9` 统一围绕下面 3 个对象推进：

### 1. 工作文本（working text）

- 当前 `editor` 正在编辑的内存内容

### 2. 正式落盘基线（persisted baseline）

- 最后一次成功写入正式文件后的可信内容状态

### 3. 恢复快照（recovery snapshot）

- 尚未进入正式主文件、但用于异常退出后兜底恢复的辅助内容层

## 运行态结构

```text
AppSaveState
├── working_text          # editor 当前正在编辑的文本
├── working_version       # 工作文本版本，每次真实内容修改后递增
├── persisted_version     # 最后一次正式保存成功后的版本
├── dirty                 # working_version != persisted_version
├── recovery_version      # 恢复快照对应的版本（Phase 9 引入）
├── last_edit_tick        # 最近一次真实内容修改时间（Phase 7 使用）
├── last_save_tick        # 最近一次正式保存成功时间（Phase 7 使用）
└── pending_change_bytes  # 未落盘修改压力指标（Phase 7 使用）
```

约束：

1. `working_text` 的真实持有者是 `editor`
2. `AppSaveState` 是 `app` 对保存安全的运行态抽象，不复制整份文本
3. `dirty` 由 `working_version != persisted_version` 推导
4. `persisted_version` 只在正式保存完整成功后推进

## 统一状态语义

### clean

- `working_version == persisted_version`

### dirty

- `working_version != persisted_version`

### recoverable

- `recovery_version > persisted_version`

## 状态转换规则

### 编辑成功

- `editor` 返回“内容修改”
- `working_version += 1`
- 重新计算 `dirty`

### 非内容操作

- `editor` 返回“非内容修改”或“无变化”
- `working_version` 不变
- `dirty` 不变

### 正式保存成功

- `persisted_version = working_version`
- `dirty = false`

### 正式保存失败

- `persisted_version` 不变
- `dirty` 保持 true

### 异常退出后恢复

- 先加载 `persisted baseline`
- 再检查 `recovery snapshot`
- 若 `recovery_version > persisted_version`，则由 `app` 决定是否采用恢复内容

## 工作流程

### 流程 1：启动加载

```text
platform
-> app 启动初始化
-> storage 读取正式 note.md / state.ini
-> app 把正式内容装入 editor
-> app 建立初始 AppSaveState
   - working_version = persisted_version
   - dirty = false
```

### 流程 2：普通编辑

```text
platform 收到输入
-> app 分发编辑命令
-> editor 执行命令
-> editor 返回：内容修改 / 非内容修改 / 无变化
-> app 仅在“内容修改”时推进 working_version
-> app 重新计算 dirty
```

### 流程 3：正式保存

```text
app 发起正式保存
-> storage 写正式文件
-> storage 返回成功 / 失败
-> app 成功时推进 persisted_version
-> app 重新计算 dirty
```

### 流程 4：自动保存（Phase 7）

```text
platform 提供时间事件
-> app 读取 AppSaveState
-> app 判断：
   - dirty ?
   - idle 超时 ?
   - 最长间隔超时 ?
   - pending_change_bytes 超阈值 ?
-> 满足条件才进入“正式保存流程”
```

### 流程 5：原子保存（Phase 8）

```text
app 发起正式保存
-> storage 写 temp
-> storage flush / close
-> storage replace 正式文件
-> storage 返回完整成功 / 失败
-> app 决定是否推进 persisted_version
```

### 流程 6：崩溃恢复（Phase 9）

```text
上次运行异常结束
-> recovery snapshot 保留
-> 下次启动
-> app 先加载 persisted baseline
-> app 再检查 recovery snapshot
-> app 决定是否采用恢复内容
-> 若采用，则 editor 装入恢复文本
```

## 按流程划分的模块职责

| 流程步骤 | 归属模块 |
|---|---|
| 收到键盘 / 定时器 / 关闭窗口事件 | `platform` |
| 决定调用哪个编辑或保存流程 | `app` |
| 修改工作文本 | `editor` |
| 判断这次是否真的改了文本 | `editor` |
| 推进 `working_version / persisted_version / dirty` | `app` |
| 正式文件读写 | `storage` |
| 恢复快照读写 | `storage` |
| 决定是否采用恢复快照 | `app` |

## Phase 6-9 各自负责什么

### Phase 6

- 定义“已提交 / 未提交”
- 建立 `working_version / persisted_version / dirty`
- 收紧 `editor -> app` 的内容修改语义
- 收紧正式保存成功 / 失败后的状态转换

### Phase 7

- 定义“什么时候提交”
- 基于保存模型做 idle / 最长间隔 / 未落盘压力调度
- 不改原子写盘细节

### Phase 8

- 定义“怎么安全提交”
- 把正式保存改成 temp -> replace
- 只有完整成功后才推进 persisted_version

### Phase 9

- 定义“来不及提交时怎么兜底”
- 为未正式提交内容提供 recovery snapshot
- 恢复判定建立在 persisted baseline 之上

## 读取顺序建议

1. `./plan.md`
2. `./save-safety-model.md`
3. 当前阶段对应的 `./phases/phase-X.md`
