# Shell-1a - 壳层状态与标题栏命令模型（后续）

## 阶段摘要

把 Shell 主线从“只有需求描述”推进到“先有统一壳层状态和标题栏命令模型”的状态。

## 阶段目标

1. 建立最小壳层状态模型
2. 建立标题栏命令枚举与命令分组
3. 明确 `app` 持有的壳层运行态和 `storage` 持久化边界

## 为什么要做这个

1. 不先收束状态和命令模型，后续 frameless、titlebar、tray、topmost、AppBar 都会各自长接口
2. 标题栏按钮如果没有统一命令模型，很容易再次退化成零散按钮行为
3. 这个子阶段必须最先做，因为它是后续所有 Shell 子阶段的接口前提

## 阶段产出

1. 壳层状态字段清楚
2. 标题栏按钮不再是随手定义的零散行为
3. 后续 Shell-1b / 1c / 3a / 4a / 5a 有统一接口前提

## 本阶段范围

1. 壳层状态枚举
2. 标题栏命令模型
3. 最小持久化字段约定

## 本阶段不做

1. Frameless 窗体切换
2. 标题栏绘制
3. 拖拽 / 缩放 / 托盘 / topmost / AppBar 行为实现

## 分层归属

### `app/`

- 持有壳层状态与标题栏命令模型

### `storage/`

- 承接最小持久化字段

### `ui/`

- 后续消费命令模型，但本阶段不负责绘制

### `platform/`

- 本阶段原则上不参与实现细节

### `render/`

- 本阶段不参与

### `editor/`

- 本阶段不参与

### `core/`

- 本阶段不参与

## 文件落点

### 本阶段预计修改文件

- `src/app/app.h`
- `src/app/app.c`
- `src/storage/state_store.h`
- `src/storage/state_store.c`

### 本阶段原则上不应修改

- `src/platform/win32/window.c`
- `src/ui/editor_view.c`
- `src/render/render.c`
- `src/editor/editor.c`

## 技术路线

   ### 新增入口文件

   - `src/app/app.h`
     - 新增壳层状态与标题栏命令对外声明
   - `src/app/app.c`
     - 新增壳层状态初始化、读取和命令分发入口

   ### 状态承接文件

   - `src/app/app.c`
     - 持有运行态：
       - `ShellState`
       - `ShellCommand`
       - `TitlebarCommandGroup`
   - `src/storage/state_store.h` / `state_store.c`
     - 只承接最小持久化字段，不提前落完整 tray / resident / dock 细节

   ### 调度 / 测量 / 适配文件

   - `src/app/app.c`
     - 启动时从 `state_store` 读取最小壳层字段
     - 运行时把后续 `ui / platform` 的命令消费统一收口到 `app`

   ### 旧路径调整

   - 后续标题栏按钮、tray 命令、resident mode 命令都不再各自定义独立枚举
   - 若当前已有零散按钮语义，应统一降级为 `ShellCommand` 的消费端，而不是继续各层自定义命令名

## 主链路

```text
启动
-> storage 读取最小壳层字段
-> app 建立 ShellState
-> 后续 UI / platform 统一消费 ShellState / ShellCommand
```

## 完成标准

1. 壳层状态模型清楚
2. 标题栏命令模型清楚
3. `app` 与 `storage` 边界清楚
4. 后续子阶段无需再回头重定命令模型

## 计划的最后一步

1. `git add` 当前阶段修改过的 shell 计划文件
2. `git commit` 使用规范提交说明记录这次阶段计划调整
3. `git push` 把阶段计划更新提交到远端仓库
4. 如果当前计划确认已完成，把对应计划文件重命名为追加 `_done` 后缀的文件名
