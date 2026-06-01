# Shell-1c-repair-1 - 修复旧持久化状态把 frameless 回退为系统窗体（后续）

## 阶段摘要

把系统从“`Shell-1c_done` 主体已完成但会被旧 `state.ini` 回退”推进到“frameless 基线在启动期和持久化兼容场景下都能稳定生效”的状态。

## 阶段目标

1. 修复旧 `state.ini` 中 `use_custom_chrome=0` 把启动期 frameless 基线回退为系统窗体的问题
2. 明确壳层开关字段的兼容策略，避免旧持久化值继续静默覆盖 `Shell-1c_done` 的默认基线
3. 在不重开 `Shell-1c_done` 主体定义的前提下，让 `Shell-1d` 可以建立在稳定可见的 frameless 基线上继续推进

## 为什么要做这个

1. `Shell-1c_done` 的主链已经落地，但当前用户实际看到的窗口仍可能是系统标题栏状态，这会直接让后续 AI 和用户误判 `1c` 是否真的生效
2. 这个问题不是新的壳层能力，而是已完成阶段落地后暴露出的**兼容性回退缺陷**，单独修复更可追溯，也避免回写 `_done` 阶段
3. 如果不先修复这个问题就继续推进 `Shell-1d`，标题栏和边框视觉会搭在错误的系统窗体基线上，后续判断会持续混乱

## 阶段产出

1. 旧用户目录下已有 `state.ini` 时，应用启动后仍能进入 frameless 基线
2. `use_custom_chrome` 的默认值、缺省值、旧版本兼容值和显式关闭值之间的解释规则被固定下来
3. `Shell-1d` 可以在无需手工删状态文件的前提下，直接建立在稳定的 frameless 窗体之上

## 本阶段范围

1. `use_custom_chrome` 的启动期兼容解释
2. 壳层开关字段的持久化兼容与迁移
3. `app` 启动路径中壳层状态应用顺序的修正
4. 必要时把兼容后的结果回写为新状态，避免旧值重复触发回退

## 本阶段不做

1. 重新定义 `Shell-1c_done` 的主体范围
2. 标题栏组件和边框视觉
3. 标题栏拖拽命中、缩放、双击标题栏和最大化兼容
4. 托盘 / topmost / AppBar 行为

## 分层归属

### `platform/`

- 原则上不改 frameless 主链语义
- 只在确实需要时配合启动顺序修正

### `app/`

- 负责解释启动期壳层开关的兼容规则
- 负责决定何时应用迁移后的壳层状态
- 负责把兼容后的状态回写给 `storage`

### `storage/`

- 负责表达 `use_custom_chrome` 的兼容状态、默认状态和必要的迁移字段
- 不直接参与窗口切换

### `ui/`

- 本阶段原则上不参与

### `render/`

- 本阶段原则上不参与

### `editor/`

- 本阶段不参与

### `core/`

- 本阶段不参与

## 文件落点

### 本阶段预计修改文件

- `src/app/app.c`
- `.ai/phases/phase-shell-1c-repair-1.md`（本文件）

### 本阶段原则上不应修改

- `src/app/app.h`（不需要新增接口或字段）
- `src/storage/state_store.h`（存储层结构体不变）
- `src/storage/state_store.c`（兼容解释职责在 app 层，storage 只负责读写）
- `src/storage/storage_write.c`
- `src/storage/note_store.c` / `note_store.h`
- `src/platform/win32/nonclient.c` / `nonclient.h`
- `src/platform/win32/window.c` / `window.h`
- `src/ui/editor_view.c` / `editor_view.h`
- `src/editor/editor.c` / `editor.h`
- `src/render/render.c` / `render.h`

## 技术路线

### 主产品特性与接口保留说明

- 主产品实现中，`use_custom_chrome` 固定为 `1`，界面和配置不再暴露关闭 custom chrome 的选项。
- 但底层接口/模块（如 `Platform_SetFrameless`）保留 frameless 切换能力，便于未来扩展或子产品复用。
- 文档需注明：主线产品不支持关闭 custom chrome，接口仅为扩展预留，默认行为始终为 frameless。
- 代码实现时，主产品路径只走 frameless，任何非 frameless 路径仅作为底层能力保留，不在主产品暴露。

### 兼容规则（确定性）

主产品启动后：

> **忽略 `state.ini` 中 `use_custom_chrome` 的旧值，统一强制进入 frameless 基线，并在首次启动后回写 state.ini。**

这条规则具体展开为：

1. `App_ApplyShellStateData` **不再从 `state.ini` 消费 `use_custom_chrome`**，该字段的运行时值始终由 `App_InitShellStateDefaults` 初始化为 `1`
2. 如果 `state.ini` 中旧值 `use_custom_chrome=0`，首次启动完成加载后在 `App_LoadInitialDocument` 末尾回写为 `1`
3. 不依赖版本号判断，不做增量版本兼容推理，以最简单的"忽略旧值 → 强制 frameless → 回写修正"三元组覆盖所有场景

### 新增入口文件

- 本阶段原则上不新增新入口文件
- 继续以 `src/app/app.c` 中的启动装载主链作为修复入口

### 状态承接文件

- `src/app/app.h` / `app.c`
  - 承接兼容后的壳层开关状态，并在启动期应用
  - 运行态 `g_app.shell.use_custom_chrome` 永远为 `1`，不因旧 state.ini 回退
- `src/storage/state_store.h` / `state_store.c`
  - `use_custom_chrome` 字段在存储层保持读写能力（底层接口兼容性）
  - **app 层不再消费该字段的读结果**，`state_store` 不承担兼容解释职责

### 调度 / 测量 / 适配文件

- `src/app/app.c`
  - **`App_ApplyShellStateData`**：删除 `g_app.shell.use_custom_chrome = state->use_custom_chrome ? 1 : 0;` 这一行
  - **`App_LoadInitialDocument`**：在 `App_TryLoadRecoverySnapshot()` 返回后追加迁移回写块——检测 `state.use_custom_chrome != 1` 时重新构造 `StateData` 并调用 `StateStore_Save` 写入 `use_custom_chrome=1`
  - 执行顺序变为：`默认值 -> 读取旧状态（跳过 use_custom_chrome） -> 应用壳层开关 -> 检测旧值并回写修正`
- `src/storage/state_store.c`
  - 原则上不改。兼容解释职责不在存储层，存储层只负责读写字段本身

### 旧路径调整

以下旧路径按确定性兼容规则替换：

1. **`App_ApplyShellStateData` 消费 `use_custom_chrome` 的路径** → 删除，app 层不再从 state.ini 读取该字段
2. **`state.ini` 中 `use_custom_chrome=0` 静默覆盖运行态** → 替换为 app 层强制固定为 `1`
3. **手工删除 `state.ini` 才能看见 frameless 的隐式恢复路径** → 移除（不再需要，因为旧值不再生效）
4. **迁移回写路径** → 新增：首次加载时检测旧值并回写 `use_custom_chrome=1`

`Shell-1c_done` 中已经完成的 frameless / client-area 主链保持不动，本阶段只修正启动兼容和状态迁移，不重开主体实现。

## 主链路

```text
程序启动
-> storage 读取 state.ini 到 StateData
-> app 将 StateData 中 use_custom_chrome 以外字段应用到 g_app.shell（App_ApplyShellStateData 跳过 use_custom_chrome）
-> app 应用壳层开关（App_ApplyShellChromeState -> Platform_SetFrameless(1)）
-> 如果 state.ini 中的旧值 use_custom_chrome != 1
   -> app 构造新 StateData，写入 use_custom_chrome=1
   -> storage 回写 state.ini
```

## 完成标准

1. 即使用户目录里存在旧 `state.ini`，启动后主窗口仍进入 frameless 基线
2. `use_custom_chrome` 的默认值、显式关闭值和旧版本兼容值不会再互相混淆
3. 不需要手工删除状态文件，就能看到 `Shell-1c_done` 已实现的 frameless 结果
4. `Shell-1d` 可以直接在修复后的启动基线上继续推进

## 计划的最后一步

1. `git add` 当前阶段修改过的 shell 计划文件
2. `git commit` 使用规范提交说明记录这次阶段计划调整
3. `git push` 把阶段计划更新提交到远端仓库
4. 如果当前计划确认已完成，把对应计划文件重命名为追加 `_done` 后缀的文件名
