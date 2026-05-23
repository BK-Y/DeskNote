# 阶段 0～1：环境搭建 + 第一个窗口 ✅ 已完成

## 阶段 0：环境搭建 (Day 1) ✅ 已完成

**目标**：用全开源工具链编译出第一个 Hello World

### 工具链（全部开源）

| 工具 | 用途 |
|------|------|
| MSYS2 | Unix 风格构建环境 + 包管理器 |
| MinGW-w64 (GCC) | C/C++ 编译器 |
| CMake | 跨平台构建系统 |

### 步骤

| 步骤 | 内容 | 学习点 |
|------|------|--------|
| 0.1 | 下载安装 MSYS2，运行 pacman -Syu 更新 | 包管理器是什么 |
| 0.2 | pacman -S mingw-w64-ucrt-x86_64-gcc cmake git | 编译器、构建工具 |
| 0.3 | 将 C:\\msys64\\ucrt64\\bin 加入系统 PATH | 环境变量的作用 |
| 0.4 | 验证：gcc --version / cmake --version | 确认安装成功 |
| 0.5 | 创建 main.cpp + CMakeLists.txt | 头文件 vs 源文件、#include |
| 0.6 | WinMain 输出 MessageBox | 程序入口点、Unicode |
| 0.7 | cmake -B build -G "MinGW Makefiles" && cmake --build build | 编译 → 链接流程 |
| 0.8 | 里程碑：desknote.exe 弹窗成功 |

### C/C++ 知识点
- 源文件 .c / .cpp 的区别
- 编译 → 链接的完整流程
- WinMain vs main（GUI 程序入口）
- GCC runtime exception：MinGW 编译的 exe 可以闭源发布

---

## 阶段 1：第一个窗口 (Day 2-3) ✅ 已完成

**目标**：创建可拖拽、可关闭、可置顶、可靠边的无边框便签窗口

### 步骤

| 步骤 | 状态 | 内容 | 学习点 |
|------|------|------|--------|
| 1.1 | ✅ | 注册窗口类 WNDCLASSEX，创建窗口 CreateWindowEx | 消息循环、窗口过程 |
| 1.2 | ✅ | WS_POPUP 无边框，自绘黄色标题栏 | 窗口样式位掩码、WM_PAINT |
| 1.3 | ✅ | 拖拽移动 SendMessage(HTCAPTION) | 鼠标消息、HIWORD/LOWORD |
| 1.4 | ✅ | 关闭按钮点击响应 | 按钮区域检测、DestroyWindow |
| 1.4b | ✅ | 标题栏拆分为独立文件 titlebar.h / .c | 模块拆分、接口设计 |
| 1.4c | ✅ | 关闭按钮悬停变红 | WM_MOUSEMOVE、TrackMouseEvent |
| 1.4d | ✅ | 添加应用图标（.ico + .rc） | 资源脚本、LoadIcon |
| 1.5 | ✅ | 右下角 resize 拖拽 | WM_NCHITTEST、HTBOTTOMRIGHT |
| 1.6 | ✅ | 置顶按钮（图钉图标） | SetWindowPos(HWND_TOPMOST) |
| 1.7 | ✅ | 靠边隐藏（鼠标离开时自动吸附） | WM_MOUSELEAVE、SetTimer |
| 1.8 | ✅ | 里程碑：完整可用的便签窗口 | |

### 靠边隐藏流程

| 步骤 | 内容 |
|------|------|
| 1.7a | 鼠标离开窗口时触发 WM_MOUSELEAVE |
| 1.7b | Window_CheckAndAutoHide 检测窗口是否在屏幕边缘 |
| 1.7c | 是则 SetWindowPos 移出屏幕（留 3px 色条） |
| 1.7d | 启动定时器 SetTimer(200ms) 检测鼠标 |
| 1.7e | WM_TIMER 中 GetCursorPos 检查鼠标是否靠近色条 |
| 1.7f | 鼠标靠近时 Window_PopOut 滑回原位 |
| 1.7g | 隐藏按钮可手动切换隐藏/弹出 |

### C/C++ 知识点
- struct 结构体初始化
- 回调函数、LRESULT CALLBACK
- Win32 消息泵 GetMessage / DispatchMessage
- WPARAM / LPARAM 是什么
- SetWindowPos Z-order（HWND_TOPMOST）
- SetTimer / KillTimer 定时器
- GetCursorPos 获取鼠标坐标
- WM_PAINT、BeginPaint / EndPaint 绘图流程
- HDC、CreateSolidBrush、FillRect GDI 绘图
- HIWORD / LOWORD 从 LPARAM 提取坐标
- InvalidateRect 强制重绘
- 头文件 .h 与源文件 .c 的分离
- 多文件编译，CMakeLists.txt 添加新源文件
- .rc 资源脚本文件格式、windres
