# phase-shell-1c — 已完成

完成日期: 2026-05-27

概述:

- 本阶段已把 frameless / client-area 主链正式接通：
  - `src/platform/win32/window.c` 已可按壳层开关切换到 frameless 样式
  - `src/platform/win32/nonclient.c` 已承接最小 non-client 指标与内容区计算
  - `src/app/app.c` 已把壳层开关应用、客户区重建、内容区重算收口到统一主链
  - `src/ui/editor_view.c` 已改成消费内容区矩形合同，而不是默认整块客户区就是正文区
- 本阶段的“完成”含义固定为：**frameless 主体实现已落地**，而不是所有启动兼容性问题都已关闭。
- 在完成后暴露出的“旧 `state.ini` 中 `use_custom_chrome=0` 会把 frameless 回退成系统窗体”的问题，不再回写本阶段，而是交给新增的 `phase-shell-1c-repair-1.md` 单独修复。

验收结果:

- frameless 样式切换链已经存在
- client-area 重建链已经存在
- `editor_view` 已不再默认整块客户区等于正文区

遗留修复项:

- 旧持久化状态可把启动期 frameless 基线回退为普通系统窗体
- 该问题属于 `Shell-1c` 完成后暴露出来的兼容性修复，不再回写本文件
- 后续统一由 `Shell-1c-repair-1` 承接

注: 此文件进入冻结态，只用于记录 `Shell-1c` 已完成的主体范围。
