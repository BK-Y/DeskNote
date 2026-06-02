# 阶段 2～4：编辑、存储与读写（历史归档）

> **状态：已完成，但本文已不再作为当前执行计划。**  
> 当前阶段计划请查看 `.ai\plan.md` 与 `.ai\phases\phase-5.md`。

## 这几个阶段在当前代码里的实际落点

### 阶段 2：最小文本编辑

已经落地的模块：

- `src\core\document.*`
- `src\editor\editor.*`
- `src\render\render.*`
- `src\ui\editor_view.*`
- `src\platform\win32\window.*`
- `src\app\app.*`

已经具备的能力：

- 文本输入
- 回车 / 退格
- 左右方向键移动
- DirectWrite 光标定位
- 系统光标与 IME 跟随

### 阶段 3：本地状态存储

已经落地的模块：

- `src\storage\state_store.*`

当前方案：

```text
%LOCALAPPDATA%\DeskNote\state.ini
```

当前状态文件只保存最小运行状态，主要是 `last_file`。

### 阶段 4：默认便签读写

已经落地的模块：

- `src\storage\note_store.*`
- `App_LoadInitialDocument()`
- `App_SaveCurrentDocument()`

当前方案：

```text
%LOCALAPPDATA%\DeskNote\note.md
```

当前能力：

- 启动时加载默认便签
- 关闭时保存当前便签
- 同时写回 `state.ini`

## 已经放弃的旧设想

下面这些内容属于早期方案，当前已经不再采用：

- `storage.c` / `notefile.c` 旧模块划分
- `%APPDATA%\DeskNote\notes\` + `settings.json`
- `dn_YYYYMMDD_###.md` 命名主线
- 右键菜单 / 设置面板作为这一阶段的核心目标

这些能力未来如果需要回归，会在新的阶段里重新定义，而不会直接沿用这份旧拆解。

## 当前下一步

阶段 2～4 完成后，当前下一步不是继续扩展旧存储方案，而是：

1. 点击定位光标
2. 最小垂直浏览
3. 光标与视口联动
