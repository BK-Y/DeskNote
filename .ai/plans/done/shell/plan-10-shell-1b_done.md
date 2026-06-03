# phase-shell-1b — 已完成（最小 non-client 骨架接通）

完成日期: 2026-05-27

概述:

- 本阶段实现并集成了最小的 non-client 骨架（`src/platform/win32/nonclient.c` / `nonclient.h`）。
- 在 `src/platform/win32/window.c` 中：包含 `nonclient.h`，在窗口创建后调用 `Platform_Nonclient_Init(hwnd)`，将 `WM_NCHITTEST` 转发给 `Platform_Nonclient_HandleNCHitTest`，在 `WM_DESTROY` 时调用 `Platform_Nonclient_Shutdown(hwnd)`。
- 将 `nonclient.c` 加入 `CMakeLists.txt` 并完成本地构建（生成可执行文件）。
- 本阶段只保留“最小 non-client 骨架 + 顶区拖拽占位”这一层完成结果；**完整的 frameless 窗体切换与客户区重建并不在这里宣告完成**，而是下沉到新的 `Shell-1c` 单独推进。
- 后续标题栏组件与边框视觉基线应交给 `Shell-1d`，标题栏命中分区、正式拖拽语义、缩放热区与最大化兼容应分别交给 `Shell-2a / 2b` 收口，不能在后续阶段再回头把 `Shell-1b` 重做一遍。

验收标准:

- `WM_NCHITTEST` 在窗口顶部区域返回 `HTCAPTION`，支持拖动（已验证）。
- 项目能够通过 CMake 构建并启动基本 UI（已验证）。

未完成/后续项:

- 目前仅保留最小顶区 `HTCAPTION` 占位行为，用于证明 non-client 骨架已接通，而不是完整 frameless 已完成。
- 后续应由 `Shell-1c` 承接 frameless 窗体切换与客户区重建。
- 后续应由 `Shell-1d` 承接标题栏组件与边框视觉基线。
- 后续应由 `Shell-2a` 正式替换当前“顶区即拖拽”的临时路径，改成标题栏命中分区与拖拽链。
- 后续应由 `Shell-2b` 承接边缘 resize hit-test（HTLEFT/HTRIGHT/...）、双击标题栏与最大化兼容。

注: 此文件作为阶段完成的记录，用于与 `phase-planning` 标准保持一致。
