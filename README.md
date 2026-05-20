# DeskNote — 桌面便签工具

## 项目目标

DeskNote 是一个桌面便签应用，目标是：

- 极致响应：用户体验上“几乎无感”，尽量把延迟与阻塞控制到最低。
- Markdown 原生支持：便签内容可通过 Markdown 语法录入并富文本渲染/导出。
- 轻量与稳定：在常用桌面平台上运行流畅，内存占用与启动时间尽可能小。
- 易扩展：模块化设计，便于替换渲染器、适配同步后端或增加插件功能。

## 快速开始（开发者）

克隆仓库并初始化子模块（包含 md4c）：

```desknote/README.md#L1-12
git clone --recurse-submodules <your-repo-url>
# 或者
# git clone <your-repo-url>
# git submodule update --init --recursive
```

本项目使用 CMake（>= 3.12）管理 C 语言代码（其中 `lib/md4c` 为 Markdown 解析器子模块）。示例构建步骤：

```desknote/README.md#L13-28
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target my_md4c_demo -j
# 运行 demo（演示 md4c 集成）
./my_md4c_demo
```

## 使用方式（用户视角）

- 启动应用后，在便签编辑区可以直接输入 Markdown 语法。编辑器应支持实时预览或按需渲染。\
- 支持常见 Markdown 功能：标题、列表、加粗/斜体、代码块、表格、图片（本地或远端链接）。\
- 支持导出为 HTML 或纯文本（可选通过 md4c-html 提供的渲染接口）。

## 项目结构（概览）

- `lib/md4c/` — 第三方 Markdown 解析器（以 submodule 方式管理）。
- `src/` — 应用核心代码（解析层、存储层、UI glue-code）。
- `docs/` — 项目文档：`docs/notes/`、`docs/plan/`、`docs/adr/`。
- `notes/` — 个人学习笔记（非必需，可用于临时记录）。

## 文档与规范

项目文档存放在 `desknote/docs/` 下：

- `desknote/docs/readme.md` — 文档规范与贡献说明。\
- `desknote/docs/notes/` — 学习笔记集合。\
- `desknote/docs/plan/` — 开发计划与里程碑。\
- `desknote/docs/adr/` — 架构决策记录（ADR）。


## License

- 项目主体许可待定（请在此处补充项目许可证）。\
- `lib/md4c` 使用 MIT 协议（请保留 `lib/md4c/LICENSE.md`）。

## 联系方式

- 维护者: Peiyang
- Git 仓库: （此处填写远端仓库地址）
