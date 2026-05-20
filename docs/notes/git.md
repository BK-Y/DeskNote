# Git 子模块（Submodule）笔记

用途与概念概述

- 子模块（submodule）是把一个 Git 仓库作为另一个仓库的子目录来管理的机制。父仓库只保存子模块的引用（即子仓库的一个具体 commit SHA），而不把子模块的代码历史合并到父仓库中。
- 简单来说：父仓库记录“子仓库应该指向哪个 commit”，子仓库本身仍然是独立的 Git 仓库。

为何使用子模块（优点）

- 可复现性：父仓库记录确切的子模块 commit（SHA），他人 clone 父仓库并初始化子模块后能得到完全相同的子模块版本，便于构建可复现。
- 清晰的边界：第三方依赖代码不混入主仓库历史，便于审查和升级。
- 保留 upstream 信息：子模块保持为一个完整的上游仓库（包含 tags/branches/history），方便查阅来源、发 PR 或同步改动。
- 与构建系统友好：对于使用 upstream 自带构建脚本（例如 CMake 的 add_subdirectory）的项目，submodule 可以直接复用上游的构建配置。

子模块的基本行为（概念）

- 父仓库在其 commit 中保存子模块的一个指针（gitlink），该指针是子模块仓库的具体 commit SHA。
- 父仓库不会自动更新该指针；当上游子模块仓库有更新时，父仓库仍然保持指向旧的 SHA，除非你显式在本地更新子模块并在父仓库 commit 新的指针。
- 子模块通常会以 detached HEAD 的形式被检出（因为父仓库锁定的是 SHA 不是分支名）。如果在子模块上开发，应先创建并切换到新分支。

常用命令（最小集合）

- 添加子模块：

```desknote/notes/git.md#L200-240
git submodule add <repo-url> path/to/submodule
```

- 初始化并拉取（已存在父仓库但未下载子模块）：

```desknote/notes/git.md#L240-260
git submodule update --init --recursive
```

- 一次性 clone 并带上子模块：

```desknote/notes/git.md#L260-280
git clone --recurse-submodules <parent-repo-url>
```

- 查看子模块状态：

```desknote/notes/git.md#L280-300
git submodule status
git submodule foreach 'git status --porcelain; git rev-parse --abbrev-ref HEAD || git rev-parse --short HEAD'
```

- 在子模块中开发并让父仓库记录新指针（工作流程示例）：

```desknote/notes/git.md#L300-340
cd path/to/submodule
# 在子模块创建分支并提交改动（在子模块里）
git checkout -b my-fix
# 修改、git add、git commit、git push 到子模块远端

# 回到父仓库并记录新指针
cd ../..
git add path/to/submodule
git commit -m "chore(submodule): bump submodule to include my-fix"
git push
```

- 更新子模块指针以跟随上游远端（显式操作）：

```desknote/notes/git.md#L340-360
# 在父仓库层面把子模块更新到远端跟踪分支的最新 commit
git submodule update --remote path/to/submodule
git add path/to/submodule
git commit -m "chore(submodule): update to remote HEAD"
git push
```

- 如果你只想浅拉子模块以节省流量：

```desknote/notes/git.md#L360-380
git submodule update --init --recursive --depth 1
```

常见问题与排查要点

- 子模块未初始化：`git submodule update --init --recursive`。
- 子模块处于 detached HEAD：父仓库记录的是具体的 SHA，进入子模块开发请创建分支：`git checkout -b my-branch`。
- 子模块更新后别人拿不到新 commit：确保在子模块内的 commit 已经 `git push` 到子模块远端，然后在父仓库 `git add path/to/submodule` 并 `git commit` 指针。
- HTTPS TLS / GnuTLS 错误：改用 SSH URL（`git@github.com:owner/repo.git`）或诊断网络/代理问题。

协作与 CI 建议

- README/CONTRIBUTING 中写明：如何 clone 并初始化子模块，例如：

```desknote/notes/git.md#L420-440
# 推荐命令
git clone --recurse-submodules <repo>
# 或
git clone <repo>
git submodule update --init --recursive
```

- 在 CI（例如 GitHub Actions）中确保 checkout 时初始化子模块：

```desknote/notes/git.md#L440-460
- uses: actions/checkout@v4
  with:
    submodules: true
    fetch-depth: 0
```

替代方案（何时不用 submodule）

- Vendor（直接把第三方源码拷贝到 `vendor/`）：适用于需要打包成独立发行物或在离线环境构建的场景。升级麻烦但对使用者透明。
- Git subtree：把上游仓库的内容合并到父仓库历史中，既保留上游内容又方便同步；适合你想把第三方的历史纳入主仓库的场景。
- 系统依赖（pkg-config / find_package）：在目标平台可用 system package 时，优先使用系统包可以减少源代码依赖在仓库中。

最佳实践（简短）

- Pin 到 tag/commit：在子模块目录 checkout 到稳定 tag（如 `vX.Y.Z`），再在父仓库 commit 指针，保证构建可复现。
- 文档化：在 README 里写明如何 clone（推荐 `--recurse-submodules` 或显式 submodule update 命令）。
- 提交流程：在子模块里 commit & push 后，回到父仓库 `git add submodule` 并 `git commit`。两个步骤都必须做。
- CI：确保在自动化构建中初始化 submodule。推荐 actions/checkout 的 `submodules: true`。

示例工作流（简洁版）

1. 克隆主仓库并初始化子模块：

```desknote/notes/git.md#L520-540
git clone --recurse-submodules <parent-repo>
cd parent-repo
```

2. 在子模块中做改动并推送：

```desknote/notes/git.md#L540-560
cd lib/md4c
git checkout -b fix-branch
# 做改动、git add、git commit
git push origin fix-branch

# 回到父仓库并记录子模块新指针
cd ../..
git add lib/md4c
git commit -m "chore(md4c): bump submodule"
git push
```

3. 升级子模块到上游的某个 tag 并记录：

```desknote/notes/git.md#L560-580
cd lib/md4c
git fetch --tags origin
git checkout v0.5.3   # 选择 tag
cd ../..
git add lib/md4c
git commit -m "chore(md4c): pin to v0.5.3"
git push
```

结尾说明

- 这份笔记放在 `desknote/notes/git.md`，你可以随时更新或扩展。若你希望我把示例脚本（如 bootstrap.sh）一并写入 notes 或在仓库里创建测试 demo，我可以接着做。