# 阶段 2～4：编辑器 + 存储 + 文件读写

## 状态说明

> **Rich Edit 已从技术栈中移除。** 阶段 2 原计划基于 Rich Edit 的 Markdown 渲染内容已过时，不再追溯。
> 阶段 3（存储位置）和阶段 4（文件读写）的核心功能已实现在 `storage.c` / `notefile.c` 中。
> 编辑方案已确定：基于 Direct2D + DirectWrite 的单窗口 WYSIWYG，增量 AST 驱动。

---

### 阶段 2：文本编辑 (待定) ⏳

**目标**：便签里能打字

> 编辑器方案已确定：基于 Direct2D + DirectWrite 的单窗口 WYSIWYG，增量 AST 驱动。

---

### 当前已实现的存储相关功能

以下功能已经实现在 `storage.c` 和 `notefile.c` 中：

| 功能 | 文件 | 说明 |
|------|------|------|
| 存储初始化 | `storage.c` | 创建 `%APPDATA%\DeskNote\notes\` 和 `settings.json` |
| 配置读写 | `storage.c` | JSON 简易解析/序列化，支持 `last_file` 等键值 |
| 文件命名 | `notefile.c` | 格式 `dn_YYYYMMDD_###.md`，按日期+序号自动生成 |
| 文件读取 | `notefile.c` | 读取 `.md` 文件到 UTF-16，支持 UTF-8 BOM |
| 文件保存 | `notefile.c` | 以 UTF-8 BOM 格式写回 `.md` 文件 |
| 上次文件恢复 | `notefile.c` | 启动时读取 `last_file` 配置恢复上次编辑的文件 |

---

### 阶段 3：存储位置 + 右键菜单 + 设置入口 ⏳ 排期中

**目标**：便签文件有固定的存放位置，用户可通过右键菜单设置

#### 3-A：存储路径

| 步骤 | 内容 | 学习点 |
|------|------|--------|
| 3.1 | 创建 `%APPDATA%\DeskNote\` 目录 | `GetEnvironmentVariableW` + `CreateDirectoryW` |
| 3.2 | 创建 `notes\` 子目录 | `CreateDirectoryW` |
| 3.3 | 配置 `settings.json`：存储路径 + 主题 + 字号 | 手写 JSON 序列化/解析 |
| 3.4 | 启动时读取配置，无配置时用默认路径 | `GetFileAttributesW` |
| 3.5 | 默认路径 `%APPDATA%\DeskNote\notes\` | |
| 3.6 | 配置文件中存储路径改为自定义时生效 | settings.json 中修改 notes_path |

_当前进度：步骤 3.1~3.2 已实现（`Storage_Init`），3.3~3.6 待完成。_

#### 3-B：右键菜单

```
标题栏右键点击
    ↓
    ┌────────────────────────────┐
    │  设置                        │ ← ID 1001
    │  查看全部Note                │ ← ID 1002
    │  ────────────────────────── │
    │  关于 DeskNote               │ ← ID 1003
    └────────────────────────────┘
```

_当前进度：已在 main.c 中实现（`WM_RBUTTONDOWN` + `CreatePopupMenu`），功能占位。_

#### 3-C：设置弹窗

| 步骤 | 内容 |
|------|------|
| 3.10 | 右键→设置：弹出简单设置窗口 |
| 3.11 | 设置项：存储路径（当前路径 + 更改按钮） |
| 3.12 | 设置项：颜色主题（黄/粉/蓝/绿/白，单选） |
| 3.13 | 设置项：字体大小（下拉或数字输入） |
| 3.14 | 「确定」保存配置并应用 |
| 3.15 | 「取消」关闭弹窗，不保存 |

#### 3-D：关于弹窗

_当前进度：已在 main.c 中实现（`MessageBoxW` 显示版本号）。_

#### 里程碑

便签有固定存储位置、右键菜单、设置入口、关于信息。

---

### 阶段 4：文件读写 + 失焦自动保存 ⏳ 排期中

**目标**：便签内容能存盘，下次启动恢复。

**文件命名规则**：`dn_YYYYMMDD_###.md`

- `dn` = DeskNote 前缀
- `YYYYMMDD` = 创建日期（如 20250120）
- `###` = 当日序号（001, 002...），首次创建从 001 开始

示例：
- `dn_20250120_001.md` — 1月20日第一个便签
- `dn_20250120_002.md` — 1月20日第二个便签
- `dn_20250201_001.md` — 2月1日第一个便签

_当前进度：核心读写逻辑已实现，但未接入 UI（缺少编辑器控件）。_

| 步骤 | 内容 | 学习点 |
|------|------|--------|
| 4.1 | 从 settings.json 读取 `last_file` 键 | `Storage_ReadSetting` |
| 4.2 | 文件名格式 `dn_YYYYMMDD_###.md`，无文件时按日期+序号创建 | `GetLocalTime` + `FindFirstFileW` |
| 4.3 | 读取 .md 内容 → 显示到编辑器 | `ReadFile` / `MultiByteToWideChar` |
| 4.4 | 主窗口失去焦点时自动保存 | `WM_ACTIVATE` 检测 `WA_INACTIVE` |
| 4.5 | 获取编辑器内容写入 .md 文件 | `GetWindowTextW` / `WideCharToMultiByte` |
| 4.6 | 关闭窗口时保存 last_file 到配置 + 保存内容 | `Storage_WriteSetting` |
| 4.7 | 标题栏显示当前文件名 | `DrawTextW` 更新标题文字 |
| 4.8 | **里程碑**：关掉重开，内容还在；恢复上次编辑的文件 |

_当前进度：步骤 4.1~4.3、4.6 已在 notefile.c 中实现；4.4、4.5、4.7 需等编辑器控件接入后完成。_

**启动流程**（当前已实现）：
```
Storage_Init()
    ↓
Storage_ReadSetting("last_file", buf)
    ↓
├── 有记录 → 打开该文件
└── 无记录 → 创建新文件 dn_YYYYMMDD_###.md
    ↓
读取内容 → 待传入编辑器控件
```

**关闭流程**（待接入）：
```
WM_CLOSE
    ↓
获取编辑器内容
    ↓
WriteFile 写入 .md
    ↓
Storage_WriteSetting("last_file", 当前文件名)
    ↓
DestroyWindow
```
