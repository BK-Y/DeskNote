# PL-002b: Renderer（HTML Preview 与原生渲染）

## 目标
- 提供可用的 Preview 渲染后端（HTML + WebView）作为首要实现。
- 设计原生渲染接口，支持未来替换或补充（用于编辑器内嵌渲染与低延迟绘制）。

## HTML Preview（首选）
- 实现方式：DocumentModel -> HTML 转换器 或 使用 md4c-html 直接生成 HTML 后处理。
- 优点：快速实现、可复用现成 CSS/JS（highlight.js、KaTeX）、方便导出/打印。
- 注意事项：
  - HTML 生成需要可插入安全策略（XSS 防护、外部资源加载控制）。
  - 在 Windows 首选 WebView2，Linux 可使用 GTK WebKit 或 Electron 方案（视最终打包策略）。

## 原生渲染（长期目标）
- 设计 Renderer 接口：
  - `render(DocumentModel*, RenderContext*)` 返回渲染 Command 或直接绘制到 Canvas。
  - 支持部分渲染与局部重绘（按 patch）。
- 支持视口虚拟化（Viewport Rendering）：只渲染可见块，降低大型文档渲染开销。
- 图片/资源懒加载、代码高亮策略（可用外部库或在渲染层异步加载结果）。

## 可视化与样式
- CSS / 主题：为 HTML Preview 提供内置主题（light/dark）并支持用户自定义样式。
- 字体与度量：在原生渲染时考虑字体度量差异，提供统一的行高策略。

## 任务拆解与估时
- T1: DocumentModel -> HTML 转换器（1d）
- T2: HTML Preview 集成 WebView（Windows: WebView2 优先）（1d）
- T3: 安全过滤与资源控制（XSS、外链策略）（0.5d）
- T4: 设计 Renderer 接口与原型（原生绘制草案）（1.5d）
- T5: 可视区虚拟化原型（0.5d）
- T6: 集成主题与代码高亮（0.5d）

估时合计：约 5.5 天

## 验收标准
- HTML Preview 在 WebView 中能正确显示文档样式，与 md4c 的渲染输出一致（主要块级与行级结构）。
- WebView 的交互（滚动、锚点跳转）平滑且正确，样式支持主题切换。
- 原生 Renderer 接口定义明确，并能通过示例 patch 做局部更新（原型）。

## 风险与缓解
- 风险：嵌入 WebView 的平台差异与打包体积。缓解：优先实现 WebView2（Windows），Linux 使用系统 WebKit 或轻量方案；在后续评估是否切换到更轻量原生方案。
