# DeskNote 学习笔记

> 记录从零开始构建 DeskNote 过程中的每个知识点。适合 C/C++ 新手。

---

## 目录

- [1. 工具链](#1-工具链)
- [2. 编译流程](#2-编译流程)
- [3. CMake 基础](#3-cmake-基础)
- [4. Win32 窗口模型](#4-win32-窗口模型)
- [5. 关键概念速查](#5-关键概念速查)
  - [5.1 WinMain vs main](#51-winmain-vs-main)
  - [5.2 Unicode 和宽字符](#52-unicode-和宽字符)
  - [5.3 句柄](#53-句柄)
  - [5.4 目标文件](#54-目标文件-o--obj)
  - [5.5 调用约定](#55-调用约定-winapi--callback)
- [6. 术语表](#6-术语表)

---

## 1. 工具链

### 1.1 什么是工具链？

```
源代码 (.cpp)  --→ [编译器] --→ 目标文件 (.o) --→ [链接器] --→ 可执行文件 (.exe)
                        ↑                            ↑
                     gcc/g++                      ld (由gcc自动调用)
```

| 工具 | 我们用的 | 作用 |
|------|----------|------|
| **编译器** | GCC 14.2.0 (MinGW-w64) | 把 .cpp 翻译成机器码（.o 目标文件） |
| **链接器** | GNU ld（GCC 自动调用） | 把多个 .o + 系统库拼成完整 .exe |
| **构建系统** | CMake 3.30.4 | 生成 Makefile，管理编译规则 |
| **包管理器** | MSYS2 (pacman) | 安装/更新编译工具 |

### 1.2 为什么选 MinGW-w64？

| 方案 | 开源 | 编译产物 | 运行时依赖 |
|------|------|----------|------------|
| MinGW-w64 (GCC) | ✅ GPL+exception | 独立 exe | 无（静态链接） |
| Visual Studio (MSVC) | ❌ 闭源 | 独立 exe | 可能需要 VC Runtime |
| Clang/LLVM | ✅ Apache 2.0 | 独立 exe | Windows 上通常还需 MinGW 头文件 |

MinGW-w64 的优势：
- **全开源**，零商业授权问题
- GCC 的 **runtime exception** 允许你发布闭源 exe
- 生成的 exe **不依赖额外 DLL**，一个文件即可运行
- 原生支持 Win32 API 头文件

### 1.3 UCRT 是什么？

传统 MinGW 链接 `msvcrt.dll`（老旧且不完整），现代用 **UCRT**（Universal C Runtime）——Windows 10/11 内置的 C 运行时库。

```
mingw-w64-ucrt-x86_64-gcc
         ↑
      这不是 MSVC，是 GCC 链接到 UCRT
      比旧版 msvcrt 更标准、bug 更少
```

### 1.4 环境变量 PATH 的作用

当你在终端敲 `gcc`，系统如何找到它？

```
终端输入 gcc
    ↓
系统遍历 PATH 中的每个目录
    ↓
在 C:\msys64\ucrt64\bin\ 找到 gcc.exe
    ↓
执行
```

所以必须把 MinGW 的 `bin/` 加入 PATH。

---

## 1.5 LRESULT 是什么？

`LRESULT` = **L**ong **Result**，就是 `long` 类型的别名。

```cpp
typedef long LONG_PTR;
typedef LONG_PTR LRESULT;
```

作为 `WndProc` 的返回值，告诉 Windows 消息处理结果：

| 返回值 | 含义 |
|--------|------|
| `return 0;` | 我处理了这个消息，你不用管了 |
| `return DefWindowProc(...)` | 我没处理，你按默认办 |

### 1.6 什么是函数指针？

`wc.lpfnWndProc = WndProc;` 中，`WndProc` 就是函数指针：

- 函数名就是它的内存地址（指针）
- `lpfnWndProc` 是 `WNDCLASSEXW` 结构体中的一个字段，类型是函数指针
- 你把 `WndProc` 的函数地址填进去
- Windows 注册时记下这个地址，以后有消息就通过这个地址调用你的函数

```cpp
// lpfnWndProc 的类型大概是这样的：
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
```

---

## 2. 编译流程

### 2.1 两步走

```bash
# 第1步：配置（生成 Makefile）
cmake -B build -G "MinGW Makefiles"

# 第2步：编译（按 Makefile 规则构建）
cmake --build build
```

### 2.2 为什么分两步？

| 步骤 | 执行频率 | 作用 |
|------|----------|------|
| `cmake -B build` | 只做一次（CMakeLists.txt 改了才重做） | 检测编译器、解析依赖、生成 Makefile |
| `cmake --build build` | 每次改代码后 | 编译改动过的文件 → 链接 → 输出 exe |

分开的好处：配置慢但只做一次，编译快可反复执行。

### 2.3 cmake -B build 做了什么？

1. 调用编译器检测（`gcc --version` 等），确认工具链可用
2. 解析 `CMakeLists.txt`，构建依赖图
3. 生成 `build/Makefile`

| 参数 | 含义 |
|------|------|
| `-B build` | 输出到 `build/` 目录 |
| `-G "MinGW Makefiles"` | 生成 MinGW 格式 Makefile（非 VS 的 .sln） |

### 2.4 cmake --build build 做了什么？

1. 调用 `mingw32-make`
2. make 比较 `.cpp` 和 `.o` 的时间戳
3. 只编译改动过的文件 → 链接 → 输出 `desknote.exe`

### 2.5 编译输出解读

```
[ 50%] Building CXX object CMakeFiles/desknote.dir/src/main.cpp.obj
[100%] Linking CXX executable desknote.exe
```

| 输出 | 含义 |
|------|------|
| `[50%]` | 进度（1个源文件：编译50% + 链接50%） |
| `Building CXX object` | 把 `main.cpp` 编译成 `.obj` 目标文件 |
| `Linking CXX executable` | 把目标文件 + 系统库链接成 exe |

---

## 3. CMake 基础

### 3.1 CMakeLists.txt 逐行解释

```cmake
# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.20)

# 项目名、版本、语言（CXX = C++）
project(DeskNote VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 全局宏：使用 Unicode（宽字符）API
add_definitions(-DUNICODE -D_UNICODE)

# 源文件列表
set(SOURCES
    src/main.cpp
)

# WIN32 = 无控制台窗口的 GUI 程序
add_executable(desknote WIN32 ${SOURCES})

# 链接 Windows 系统库
target_link_libraries(desknote
    user32      # 窗口/消息
    gdi32       # 绘图
    comctl32    # 通用控件
    comdlg32    # 文件对话框
    shell32     # Shell API / 托盘
)
```

### 3.2 WIN32 标识

| 不加 WIN32 | 加 WIN32 |
|------------|----------|
| 启动时弹出黑色控制台窗口 | 纯 GUI，无控制台 |
| 入口可以是 `main()` | 入口必须是 `WinMain()` |

### 3.3 系统库从哪来

Windows 自带，无需下载：

| 库 | 运行时文件 | 提供的能力 |
|----|-----------|------------|
| user32 | user32.dll | 窗口创建、消息处理、MessageBox |
| gdi32 | gdi32.dll | 绘图、画刷、画笔 |
| comctl32 | comctl32.dll | 通用控件（Rich Edit 等） |
| comdlg32 | comdlg32.dll | 文件打开/保存对话框 |
| shell32 | shell32.dll | 系统托盘、文件夹路径 |

MinGW-w64 提供对应的 `.a` 导入库（链接时用），运行时 exe 直接调用系统 DLL。

---

## 4. Win32 窗口模型

### 4.1 窗口的一生

```
注册窗口类 ──→ 创建窗口实例 ──→ [消息循环] ──→ 销毁
    ↓               ↓               ↓              ↓
WNDCLASSEX    CreateWindowEx   GetMessage    WM_DESTROY
```

### 4.2 窗口类 (Window Class)

```cpp
WNDCLASSEX wc = {};
wc.lpfnWndProc   = WndProc;        // 窗口过程函数
wc.hInstance     = hInstance;       // 程序实例句柄
wc.lpszClassName = L"DeskNote";     // 类名字符串
```

不是 C++ class，是 Win32 的"窗口类型模板"。注册后，Windows 知道有一种窗口类型叫 DeskNote，其行为由 WndProc 函数定义。

### 4.3 消息循环 (Message Loop)

```cpp
MSG msg;
while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);    // 键盘消息转换
    DispatchMessage(&msg);     // 分发给窗口过程
}
```

Windows 是**事件驱动**的。程序不主动做事，等待 Windows 发消息（鼠标、键盘、绘制），然后响应。

### 4.4 窗口过程 (Window Procedure)

```cpp
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT:    /* 需要重绘 */   break;
        case WM_CLOSE:    /* 点关闭按钮 */ break;
        case WM_DESTROY:  /* 窗口销毁 */   break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);  // 默认处理
}
```

你定义的函数，**Windows 调用它**（回调模式）。四个参数：

| 参数 | 含义 |
|------|------|
| `hwnd` | 哪个窗口收到消息 |
| `msg` | 消息类型（`WM_PAINT`、`WM_CLOSE` 等） |
| `wp` | 附加信息1（word parameter） |
| `lp` | 附加信息2（long parameter） |

### 4.5 消息流示意

```
用户点击鼠标
    ↓
Windows 内核捕获 → 生成 WM_LBUTTONDOWN → 放入消息队列
    ↓
GetMessage() 取出
    ↓
DispatchMessage() 调用你的 WndProc(hwnd, WM_LBUTTONDOWN, ...)
    ↓
你在 WndProc 中 switch-case 处理
    ↓
return 0; 告诉 Windows "已处理"
```

### 4.6 WNDCLASSEXW 各字段详解

```cpp
WNDCLASSEXW wc = { sizeof(wc) };   // cbSize：结构体版本号
wc.style         = CS_HREDRAW | CS_VREDRAW; // 窗口变宽/变高时自动重绘
wc.lpfnWndProc   = WndProc;         // ★ 消息处理函数指针（核心）
wc.hInstance     = hInstance;       // 当前 exe 的实例句柄
wc.hCursor       = LoadCursorW(NULL, IDC_ARROW); // 窗口上鼠标的样式
wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);   // 背景刷子（+1 是 Win32 惯例）
wc.lpszClassName = L"DeskNote";    // 窗口类名（创建窗口时用这个名引用）
```

### 4.7 四步缺一不可

```
① RegisterClassExW ──→ 缺：CreateWindowExW 会失败
② CreateWindowExW  ──→ 缺：没有窗口
③ ShowWindow       ──→ 缺：窗口创建了但看不见
④ Message Loop     ──→ 缺：窗口一闪就消失，进程立即退出
```

第一次写的代码缺了 ②③④，所以窗口没出现。

### 4.8 为什么消息循环是 "心脏"

```cpp
while (GetMessageW(&msg, NULL, 0, 0))
{
    TranslateMessage(&msg);   // 键盘码 → 字符消息
    DispatchMessageW(&msg);   // 发给 WndProc 处理
}
```

| 现象 | 原因 |
|------|------|
| 没循环，窗口刚开就关 | 没有 while 循环，main 函数直接跑到 return 0 |
| 窗口缩小/移动后内容消失 | 没有处理 `WM_PAINT`，系统没有收到重绘指令 |
| 关不掉窗口（进程常驻） | 没有处理 `WM_DESTROY`，没调用 `PostQuitMessage` |
| 窗口卡住动不了 | `WndProc` 里做了耗时操作没返回，消息被阻塞了 |

---

## 5. 关键概念速查

### 5.1 WinMain vs main

```cpp
int main()            → 控制台程序入口（有黑窗口）
int WINAPI WinMain()  → GUI 程序入口（无黑窗口）
```

| 参数 | 含义 |
|------|------|
| `hInstance` | 当前程序实例句柄 |
| `hPrevInstance` | 已废弃，始终 NULL |
| `lpCmdLine` | 命令行参数 |
| `nCmdShow` | 窗口初始显示方式 |

### 5.2 Unicode 和宽字符

```cpp
MessageBoxA(NULL, "ANSI 不支持中文", ...);    // A 版
MessageBoxW(NULL, L"UTF-16 支持所有语言", ...); // W 版

// 加 -DUNICODE 后 MessageBox 自动展开为 MessageBoxW
MessageBox(NULL, L"hello", L"title", MB_OK);
```

`L"字符串"` 是宽字符字面量，每个字符 2 字节（UTF-16），Windows 内核原生使用。

### 5.3 句柄

| 类型 | 含义 |
|------|------|
| `HINSTANCE` | 程序实例句柄 |
| `HWND` | 窗口句柄 |
| `HDC` | 设备上下文（绘图用） |
| `HMENU` | 菜单句柄 |

> **句柄的本质**：Windows 内部对象表的索引。你不直接操作对象，通过句柄让 Windows 替你操作——就像拿工号让前台办事，不需要直接接触对象本身。

### 5.4 目标文件 (.o / .obj)

编译的中间产物：包含机器码但没有最终地址。一个 `.cpp` 生成一个 `.o`，多个 `.o` + 库 → 链接 → `.exe`。

### 5.5 调用约定 (WINAPI / CALLBACK)

```cpp
#define WINAPI   __stdcall
#define CALLBACK __stdcall
```

`__stdcall`：参数从右往左入栈，被调用者清理栈。Win32 API 全部使用此约定。

---

## 6. 术语表

| 术语 | 英文 | 解释 |
|------|------|------|
| 编译器 | Compiler | 源码 → 机器码 |
| 链接器 | Linker | 目标文件 + 库 → 可执行文件 |
| 构建系统 | Build System | 管理编译规则的元工具（CMake） |
| 工具链 | Toolchain | 编译器 + 链接器 + 标准库的集合 |
| 运行时库 | Runtime | 程序运行时依赖的基础函数 |
| 静态链接 | Static Linking | 库代码复制进 exe，不需要外部 DLL |
| 动态链接 | Dynamic Linking | exe 运行时加载系统 DLL |
| 句柄 | Handle | Windows 内部对象的 ID（工号） |
| 消息泵 | Message Pump | 消息循环：取消息→分发→处理的循环 |
| 回调函数 | Callback | 你定义的函数，由系统/框架调用 |
| Unicode | - | 统一字符编码，Windows 用 UTF-16 |
| UCRT | Universal C Runtime | Win10+ 的现代 C 运行时库 |
| 调用约定 | Calling Convention | 函数参数入栈顺序和栈清理规则 |

---

## 7. 窗口样式速查

### 7.1 常用窗口样式 (WS_)

| 宏 | 值 | 作用 |
|----|------|------|
| `WS_POPUP` | 0x80000000 | 无边框无标题栏，一块白板 |
| `WS_CAPTION` | 0x00C00000 | 标题栏（`WS_BORDER \| WS_DLGFRAME`） |
| `WS_SYSMENU` | 0x00080000 | 系统菜单（标题栏左侧图标） |
| `WS_THICKFRAME` | 0x00040000 | 可拖拽改变大小的粗边框 |
| `WS_MINIMIZEBOX` | 0x00020000 | 最小化按钮 |
| `WS_MAXIMIZEBOX` | 0x00010000 | 最大化按钮 |
| `WS_BORDER` | 0x00800000 | 细边框（不可调整大小） |
| `WS_VISIBLE` | 0x10000000 | 窗口创建后可见（等价于之后调 ShowWindow） |
| `WS_CHILD` | 0x40000000 | 子窗口（嵌入另一个窗口内部） |
| `WS_OVERLAPPED` | 0x00000000 | 最基本的窗口（值就是 0） |

### 7.2 组合宏拆解

```cpp
// 这些宏是多个 WS_ 的按位或，实际用的时候可以自己组合想要的片段
#define WS_OVERLAPPEDWINDOW  (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | \
                              WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW       (WS_POPUP | WS_BORDER | WS_SYSMENU)
```

便签不需要系统标题栏，所以用 `WS_POPUP | WS_THICKFRAME`：
- `WS_POPUP`：去掉标题栏和边框
- `WS_THICKFRAME`：保留粗边框，用于鼠标边缘检测 resize

### 7.3 窗口类样式 (CS_)

| 宏 | 含义 |
|----|------|
| `CS_HREDRAW` | 窗口宽度变化时触发 WM_PAINT 重绘 |
| `CS_VREDRAW` | 窗口高度变化时触发 WM_PAINT 重绘 |
| `CS_DROPSHADOW` | 无边框窗口添加阴影效果（否则白板看起来像崩了） |
| `CS_DBLCLKS` | 接收鼠标双击消息（`WM_LBUTTONDBLCLK`） |

无边框窗口必须加 `CS_DROPSHADOW`，否则和桌面融为一体看不清边界。

### 7.4 扩展窗口样式 (WS_EX_)

| 宏 | 含义 |
|----|------|
| `WS_EX_TOOLWINDOW` | 不在任务栏显示窗口按钮 |
| `WS_EX_LAYERED` | 支持透明度（`SetLayeredWindowAttributes`） |
| `WS_EX_TOPMOST` | 窗口始终置顶 |
| `WS_EX_CLIENTEDGE` | 内凹边框（类似文本框的凹陷效果） |

这些在 `CreateWindowExW` 的第一个参数传入，后面会用到。

### 7.5 窗口样式和类的区别

| 层级 | 作用域 | 在哪设 |
|------|--------|--------|
| 窗口类样式 `CS_*` | 同一类的所有窗口 | `WNDCLASSEX.style` |
| 窗口样式 `WS_*` | 单个窗口 | `CreateWindowEx` 的参数 |
| 扩展样式 `WS_EX_*` | 单个窗口 | `CreateWindowEx` 的第一个参数 |

---

## 8. C vs C++ 编译对比

### 8.1 为什么可以改为 C 编译

当前代码全部是纯 C 语法：
- 没有 class、template、namespace
- 没有 new/delete、虚函数、STL
- 只有 struct + 函数 + switch-case

所以 CMakeLists.txt 中 `LANGUAGES C` 可以正常编译。

### 8.2 C 编译比 C++ 省了什么

| C++ 特有的东西 | 编译器做了什么 | 体积影响 |
|---------------|--------------|----------|
| 异常处理 `try/catch` | 生成异常展开表 `.gcc_except_table` | ~8KB |
| RTTI（运行时类型识别） | 生成类型信息字符串 `typeinfo` | ~3KB |
| 全局构造器（构造函数） | 生成 `.ctors` 初始化代码 | ~2KB |
| 模板实例化 | 生成多份函数代码 | 不定 |

当前 72KB exe 的组成：

```
exe (72KB)
├── 你写的代码（WinMain + WndProc + 绘图）    ~3KB    ← 真正的逻辑
├── 调试符号（-g 带来的）                    ~15KB   ← strip 可去掉
├── C 运行时初始化（启动代码）                ~15KB   ← 必须的
├── PE 文件头 + 导入表 + 段对齐填充           ~30KB   ← 固定开销
└── Win32 API 本身                          不在 exe 里（在系统的 DLL 中）
```

72KB 减去调试符号约 55KB，再减去 PE 固定开销约 25KB——你写的代码实际上只占 ~3KB。

### 8.3 文件后缀 .c vs .cpp

| 后缀 | 编译器行为 |
|------|-----------|
| `.c` | 按 C 语言编译 |
| `.cpp` / `.cxx` | 按 C++ 语言编译 |

CMake 判读语言方式：
- `LANGUAGES CXX` → `.cpp` 和 `.c` 都按 C++ 编译
- `LANGUAGES C` → `.c` 按 C 编译，`.cpp` 也按 C 编译（需 CMake 处理）

---

## 9. 多文件编译与模块拆分

### 9.1 为什么拆文件

当单个 .c 文件超过 ~200 行时，找代码、改代码、理逻辑都变困难。拆分的本质：
- 把不同职责的代码放到不同文件
- 每个文件只做一类事
- 文件之间通过 `.h` 头文件沟通

### 9.2 标题栏拆分示例

| 文件 | 职责 | 行数 |
|------|------|------|
| `main.c` | WinMain、消息循环、WndProc 调度 | ~110 行 |
| `ui/titlebar.h` | 声明 3 个函数接口 | ~20 行 |
| `ui/titlebar.c` | 画标题栏、检测点击 | ~50 行 |

### 9.3 头文件 (.h) 的作用

头文件是接口说明书，只写声明不写实现：

```c
// titlebar.h
#ifndef TITLEBAR_H       // 头文件保护
#define TITLEBAR_H

#include <windows.h>

void Titlebar_Draw(HWND hwnd, HDC hdc);
int  Titlebar_HitTest(HWND hwnd, int x, int y);

#endif
```

| 元素 | 作用 |
|------|------|
| `#ifndef TITLEBAR_H` | 如果没定义过 TITLEBAR_H 这个宏 |
| `#define TITLEBAR_H` | 定义它，下次再被 include 时跳过 |
| `#endif` | 结束条件编译 |
| 函数声明 | 告诉编译器有这个函数，实现在别的 .c 里 |

### 9.4 include 的双引号与尖括号

| 写法 | 搜索路径 | 用途 |
|------|----------|------|
| `#include <windows.h>` | 系统头文件目录 | 标准库、系统 API |
| `#include "titlebar.h"` | 先从当前目录找 | 自己项目里的文件 |

### 9.5 CMakeLists.txt 添加新源文件

```cmake
set(SOURCES
    src/main.c
    src/ui/titlebar.c       # 新增的源文件也要加进来
)
```

CMake 不需要手动指定 .h 文件，编译器通过 #include 自动找到。

### 9.6 这次拆分的收获

| 改动 | 变化 |
|------|------|
| main.c 行数 | 从 ~180 行降到 ~110 行 |
| WndProc 代码 | 从 80 行降到 30 行 |
| 新增文件 | 2 个（titlebar.h + titlebar.c） |
| 后续加新功能 | 只在 titlebar.c 改，不动 main.c |

---

## 10. 鼠标跟踪机制

### 10.1 系统自动触发 vs 程序手动注册

| 消息 | 触发方式 | 说明 |
|------|----------|------|
| `WM_MOUSEMOVE` | 系统自动 | 鼠标在窗口上移动就持续发送 |
| `WM_MOUSELEAVE` | 需 `TrackMouseEvent` 注册 | 鼠标离开窗口边界时触发，一次性，需重新注册 |
| `WM_MOUSEHOVER` | 需 `TrackMouseEvent` 注册 | 鼠标静止悬停一段时间后触发，一次性 |

### 10.2 为什么系统不自动发 WM_MOUSELEAVE

性能优化。如果系统每检测到鼠标离开所有窗口就发消息，会引入不必要的开销。而且大部分应用不关心鼠标离开。所以设计为：**谁关心谁注册**。

### 10.3 TrackMouseEvent 的使用模式

```c
// 1. 在 WM_MOUSEMOVE 中注册离开通知
case WM_MOUSEMOVE:
    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
    TrackMouseEvent(&tme);   // 注册一次，离开时触发一次
    break;

// 2. 在 WM_MOUSELEAVE 中处理离开事件
case WM_MOUSELEAVE:
    // 鼠标离开了，恢复悬停状态
    Titlebar_OnMouseLeave(hwnd);
    break;
```

注意：`TME_LEAVE` 是一次性的，每次 `WM_MOUSEMOVE` 中都要重新注册，否则第二次离开不会再触发。

### 10.4 悬停效果的完整流程

```
鼠标进入窗口
    ↓
WM_MOUSEMOVE 自动触发
    ↓
调用 TrackMouseEvent(TME_LEAVE) 注册离开通知
    ↓
鼠标在窗口内移动 → 持续 WM_MOUSEMOVE
    ↓
Titlebar_OnMouseMove 检测到 hit == 2
    ↓
s_hoverClose = 1，重绘标题栏 → 按钮变红
    ↓
鼠标移出关闭按钮区域
    ↓
Titlebar_OnMouseMove 检测到 hit != 2
    ↓
s_hoverClose = 0，重绘标题栏 → 按钮恢复
    ↓
鼠标移出窗口边界
    ↓
Windows 触发 WM_MOUSELEAVE
    ↓
Titlebar_OnMouseLeave → 确保 s_hoverClose = 0
```

### 10.5 关键点总结

- `WM_MOUSEMOVE` 是唯一系统自动触发的鼠标消息
- `WM_MOUSELEAVE` 必须通过 `TrackMouseEvent(TME_LEAVE)` 主动注册
- `TrackMouseEvent` 是**一次性**的，每次 `WM_MOUSEMOVE` 中都要重新调用
- `TME_LEAVE` 注册后，只有鼠标**真正离开窗口边界**时才会触发，精确有效

---

## 11. 资源文件与图标

### 11.1 什么是 .rc 文件

`.rc`（Resource Script）是 Win32 的资源脚本文件，用于定义 exe 内嵌的非代码资源：

| 资源类型 | 用途 | 示例 |
|----------|------|------|
| `ICON` | 应用图标 | `1 ICON "desknote.ico"` |
| `DIALOG` | 对话框布局 | `IDD_SETTINGS DIALOG ...` |
| `MENU` | 菜单定义 | `IDM_MAIN MENU ...` |
| `STRINGTABLE` | 字符串表 | `IDS_WELCOME "欢迎使用"` |
| `BITMAP` | 位图图片 | `1 BITMAP "logo.bmp"` |
| `VERSIONINFO` | 版本信息 | VS_VERSION_INFO |

### 11.2 资源编译流程

```
desknote.rc（文本）   →  windres（MinGW 的资源编译器）  →  desknote.res（二进制）
desknote.ico（二进制） ↗                                            ↓
                                                         链接进 exe
```

CMake 自动处理：在 `set(SOURCES)` 里加上 `.rc` 文件，CMake 会自动调用 `windres` 编译并链接。

### 11.3 .rc 文件语法

```rc
// 注释用双斜杠
// 格式：资源ID  资源类型  "文件名"

1 ICON "desknote.ico"     // ID=1 的图标，从 desknote.ico 读取
```

`1` 是资源 ID，在代码中用 `MAKEINTRESOURCEW(1)` 引用它。

### 11.4 在代码中加载图标

```c
// 在 WNDCLASSEXW 中设置窗口图标
wc.hIcon   = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
wc.hIconSm = NULL;  // 小图标，NULL 表示自动从 hIcon 缩小
```

| 参数 | 含义 |
|------|------|
| `hIcon` | 大图标（32x32），显示在 Alt+Tab 切换器和文件管理器 |
| `hIconSm` | 小图标（16x16），显示在标题栏和任务栏 |
| `LoadIconW` | 从 exe 资源中加载图标 |
| `MAKEINTRESOURCEW(1)` | 把数字 1 转换为资源 ID 字符串指针 |

### 11.5 不写 .rc 的后果

- exe 在文件管理器中显示为空白默认图标
- 任务栏和 Alt+Tab 切换器中也显示为空白图标
- 给用户的印象是"没做完的软件"

### 11.6 其他资源文件的 CMake 配置

```cmake
set(SOURCES
    src/main.c
    src/ui/titlebar.c
    resources/desknote.rc    # 直接加进来，CMake 自动处理
)
```

## 12. 窗口尺寸与定位

### 12.1 获取屏幕尺寸

```c
int screenWidth  = GetSystemMetrics(SM_CXSCREEN);  // 屏幕宽度（像素）
int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // 屏幕高度（像素）
```

### 12.2 定位到屏幕右上角

```c
CreateWindowExW(...
    GetSystemMetrics(SM_CXSCREEN) - 240, 0,   // X = 屏幕宽 - 窗口宽, Y = 0
    ...
);
```

- X 坐标 = `屏幕宽度 - 窗口宽度`，窗口右边缘贴着屏幕右边缘
- Y 坐标 = `0`，窗口贴着屏幕顶部

### 12.3 窗口尺寸的选择

Windows Sticky Notes 默认约 346×346 正方形。本便签使用 240×360，更窄但更长，适合纯文本笔记的阅读习惯。

### 12.4 标准便签风格 vs 普通窗口

| 特征 | 普通窗口 | 便签窗口 |
|------|----------|----------|
| 尺寸 | 800×600+ | 240×360（窄条形） |
| 位置 | 屏幕中央 | 右上角 |
| 标题栏 | 系统自带 | 自绘黄色 30px |
| 边框 | 有 | 无 (WS_POPUP) |

---

## 13. TRACKMOUSEEVENT 详解

### 13.1 结构体定义

```c
typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;      // 结构体大小，必须填 sizeof(TRACKMOUSEEVENT)
    DWORD dwFlags;     // 要跟踪的事件类型
    HWND  hwndTrack;   // 跟踪哪个窗口
    DWORD dwHoverTime; // 悬停触发时间（毫秒，仅 TME_HOVER 有效）
} TRACKMOUSEEVENT;
```

### 13.2 dwFlags 取值

| 标志 | 值 | 含义 |
|------|-----|------|
| `TME_LEAVE` | 0x0002 | 鼠标离开窗口边界时发 `WM_MOUSELEAVE` |
| `TME_HOVER` | 0x0001 | 鼠标静止悬停一段时间后发 `WM_MOUSEHOVER` |
| `TME_NONCLIENT` | 0x0010 | 跟踪非客户区的离开事件 |
| `TME_CANCEL` | 0x80000000 | 取消已注册的跟踪 |

### 13.3 一次性机制

`TrackMouseEvent` 注册的 TME_LEAVE 是**一次性**的：

```
第一次调用 TrackMouseEvent(TME_LEAVE)
    ↓
Windows 记录："窗口 A 要监听离开"
    ↓
鼠标移出窗口 A
    ↓
Windows 投递 WM_MOUSELEAVE
    ↓
Windows 删除该记录（一次性，用完即废）
    ↓
鼠标再次移入窗口 A
    ↓
没有重新注册 → 再次移出时不会收到 WM_MOUSELEAVE
```

所以必须在每次 `WM_MOUSEMOVE` 中重新调用 `TrackMouseEvent`：

```c
case WM_MOUSEMOVE:
    Titlebar_OnMouseMove(...);

    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
    TrackMouseEvent(&tme);   // 重新注册
    break;
```

### 13.4 为什么设计为一次性

性能考虑。如果注册是永久的，Windows 需要在鼠标离开每个窗口时都查表、投递消息。大部分应用不关心鼠标离开，一次性设计让关心的人主动重新注册，不关心的人零开销。

### 13.5 vs WM_NCHITTEST

| 消息/机制 | 触发方式 | 作用 |
|-----------|----------|------|
| `WM_NCHITTEST` | 系统自动，每次鼠标移动都发 | 定义窗口各区域的功能 |
| `TrackMouseEvent` | 程序主动注册 | 监听特定鼠标事件（离开、悬停） |
| `WM_MOUSEMOVE` | 系统自动，每次鼠标移动都发 | 响应鼠标位置变化 |

---

## 14. Win32 预定义消息大全

以下消息全部定义在 `<windows.h>` 中，以 `WM_` 开头（Window Message）。消息本质是一个 `UINT`（无符号整数），`switch-case` 中直接按名字匹配即可。

### 14.1 窗口生命周期

| 消息 | 值 | 触发时机 | 必处理？ |
|------|-----|---------|---------|
| `WM_CREATE` | 0x0001 | 窗口创建完成后、显示之前触发一次 | 可选（初始化） |
| `WM_CLOSE` | 0x0010 | 用户点了关闭按钮、或调用 `CloseWindow` / `PostMessage(WM_CLOSE)` | 推荐（DestroyWindow） |
| `WM_DESTROY` | 0x0002 | 窗口正在销毁时触发（`DestroyWindow` 调用后） | **必须**（PostQuitMessage） |
| `WM_QUIT` | 0x0012 | 消息循环收到此消息时 `GetMessage` 返回 FALSE | 不处理（系统终止消息循环） |
| `WM_NCDESTROY` | 0x0082 | 窗口销毁的最后阶段，所有子窗口已销毁 | 可选（最后清理） |

典型处理链：

```
① 用户点 × → Windows 发 WM_CLOSE
② 你调用 DestroyWindow(hwnd) → 触发 WM_DESTROY
③ 你调用 PostQuitMessage(0) → 消息循环收到 WM_QUIT → 退出
```

### 14.2 鼠标消息

| 消息 | 值 | 触发时机 | 参数 |
|------|-----|---------|------|
| `WM_MOUSEMOVE` | 0x0200 | 鼠标在窗口客户区移动 | `lp`: 低16位=X, 高16位=Y |
| `WM_LBUTTONDOWN` | 0x0201 | 鼠标左键按下 | `lp`: X, Y |
| `WM_LBUTTONUP` | 0x0202 | 鼠标左键释放 | `lp`: X, Y |
| `WM_LBUTTONDBLCLK` | 0x0203 | 鼠标左键双击 | `lp`: X, Y（需窗口类 `CS_DBLCLKS`） |
| `WM_RBUTTONDOWN` | 0x0204 | 鼠标右键按下 | `lp`: X, Y |
| `WM_RBUTTONUP` | 0x0205 | 鼠标右键释放 | `lp`: X, Y |
| `WM_MBUTTONDOWN` | 0x0207 | 鼠标中键按下 | `lp`: X, Y |
| `WM_MOUSEWHEEL` | 0x020A | 鼠标滚轮滚动 | `wp` 高16位: 滚动增量（120=向上，-120=向下） |
| `WM_MOUSELEAVE` | 0x02A3 | 鼠标离开窗口（需 TrackMouseEvent 注册） | 无额外参数 |
| `WM_MOUSEHOVER` | 0x02A1 | 鼠标悬停一段时间（需 TrackMouseEvent 注册） | `lp`: X, Y |
| `WM_NCMOUSEMOVE` | 0x00A0 | 鼠标在非客户区（标题栏/边框）移动 | `wp`: 当前所在区域（HTCAPTION 等） |

### 14.3 键盘消息

| 消息 | 值 | 触发时机 | 参数 |
|------|-----|---------|------|
| `WM_KEYDOWN` | 0x0100 | 按键按下 | `wp`: 虚拟键码（VK_LEFT、VK_ESCAPE 等） |
| `WM_KEYUP` | 0x0101 | 按键释放 | `wp`: 虚拟键码 |
| `WM_CHAR` | 0x0102 | 按键产生字符（需 TranslateMessage 转换） | `wp`: Unicode 字符码（如 'A'=65） |
| `WM_SYSKEYDOWN` | 0x0104 | 系统键按下（Alt+F4 等） | `wp`: 虚拟键码 |
| `WM_SYSKEYUP` | 0x0105 | 系统键释放 | `wp`: 虚拟键码 |

TranslateMessage 的角色：

```
WM_KEYDOWN (VK_A)    →  TranslateMessage  →  WM_CHAR ('a')
     ↑                     ↑                     ↑
原始按键                查键盘布局             得到实际字符
                        映射 QWERTY/拼音
```

### 14.4 绘制与窗口变化

| 消息 | 值 | 触发时机 | 必处理？ |
|------|-----|---------|---------|
| `WM_PAINT` | 0x000F | 窗口需要重绘（被遮挡后露出、InvalidateRect、窗口缩放） | **必须**（否则内容空白） |
| `WM_ERASEBKGND` | 0x0014 | 窗口背景需要擦除（发生在 WM_PAINT 之前） | 可选（返回 TRUE 禁止擦除） |
| `WM_SIZE` | 0x0005 | 窗口大小改变后 | 可选（更新布局） |
| `WM_MOVE` | 0x0003 | 窗口位置移动后 | 可选 |
| `WM_SIZING` | 0x0214 | 窗口正在被拖拽改变大小（持续发送） | 可选（限制最小/最大尺寸） |
| `WM_MOVING` | 0x0216 | 窗口正在被拖拽移动（持续发送） | 可选（限制移动范围） |
| `WM_GETMINMAXINFO` | 0x0024 | 获取窗口最小/最大尺寸限制 | 可选（设置最小窗口尺寸） |

WM_PAINT 的触发条件：
- 窗口从最小化恢复
- 被其他窗口遮挡后露出来
- 你主动调用 `InvalidateRect` 要求重绘
- 窗口尺寸变化（CS_HREDRAW | CS_VREDRAW 类样式时）

### 14.5 非客户区消息

| 消息 | 值 | 触发时机 | 返回值 |
|------|-----|---------|--------|
| `WM_NCHITTEST` | 0x0084 | 鼠标每次移动时发，问"鼠标在窗口哪个部位" | 返回 `HT*` 值 |
| `WM_NCLBUTTONDOWN` | 0x00A1 | 在非客户区按下左键 | 传给 DefWindowProc |
| `WM_NCPAINT` | 0x0085 | 非客户区需要重绘（标题栏/边框） | 自绘标题栏时用 |
| `WM_NCACTIVATE` | 0x0086 | 非客户区激活状态变化（窗口获得/失去焦点） | 自绘标题栏时闪烁处理 |

### 14.6 WM_NCHITTEST 返回值大全

| 返回值 | 值 | 含义 |
|--------|-----|------|
| `HTCLIENT` | 1 | 客户区（内容区域） |
| `HTCAPTION` | 2 | 标题栏（触发拖拽移动） |
| `HTCLOSE` | 20 | 关闭按钮 |
| `HTHELP` | 21 | 帮助按钮 |
| `HTMINBUTTON` | 8 | 最小化按钮 |
| `HTMAXBUTTON` | 9 | 最大化按钮 |
| `HTMENU` | 5 | 菜单栏 |
| `HTBORDER` | 18 | 不可 resize 的边框 |
| `HTLEFT` | 10 | 左边缘（可 resize） |
| `HTRIGHT` | 11 | 右边缘 |
| `HTTOP` | 12 | 上边缘 |
| `HTTOPLEFT` | 13 | 左上角 |
| `HTTOPRIGHT` | 14 | 右上角 |
| `HTBOTTOM` | 15 | 下边缘 |
| `HTBOTTOMLEFT` | 16 | 左下角 |
| `HTBOTTOMRIGHT` | 17 | **右下角**（启动 resize 拖拽） |
| `HTNOWHERE` | 0 | 窗口外部 |
| `HTTRANSPARENT` | -1 | 透明，穿透给下层窗口 |
| `HTERROR` | -2 | 错误，Windows 会发出 Beep 声 |

### 14.7 焦点与激活

| 消息 | 值 | 触发时机 |
|------|-----|---------|
| `WM_SETFOCUS` | 0x0007 | 窗口获得键盘焦点 |
| `WM_KILLFOCUS` | 0x0008 | 窗口失去键盘焦点 |
| `WM_ACTIVATE` | 0x0006 | 窗口被激活/取消激活（`wp` 低16位: WA_ACTIVE/WA_CLICKACTIVE/WA_INACTIVE） |
| `WM_ENABLE` | 0x000A | 窗口启用/禁用状态改变 |

### 14.8 定时器

| 消息 | 值 | 触发时机 | 参数 |
|------|-----|---------|------|
| `WM_TIMER` | 0x0113 | 定时器到期 | `wp`: 定时器 ID（如 `IDT_AUTOHIDE`） |

### 14.9 剪贴板

| 消息 | 值 | 触发时机 |
|------|-----|---------|
| `WM_CUT` | 0x0300 | 剪切（Ctrl+X） |
| `WM_COPY` | 0x0301 | 复制（Ctrl+C） |
| `WM_PASTE` | 0x0302 | 粘贴（Ctrl+V） |
| `WM_CLEAR` | 0x0303 | 清除（Delete） |
| `WM_UNDO` | 0x0304 | 撤销（Ctrl+Z） |

### 14.10 消息分组速记

| 分组 | 消息范围 | 包含 |
|------|---------|------|
| 窗口管理 | `0x0001` ~ `0x007F` | CREATE, DESTROY, CLOSE, MOVE, SIZE, PAINT, QUIT |
| 非客户区 | `0x0080` ~ `0x00FF` | NCHITTEST, NCACTIVATE, NCPAINT |
| 鼠标 | `0x0200` ~ `0x0209` | MOUSEMOVE, LBUTTONDOWN, LBUTTONUP 等 |
| 键盘 | `0x0100` ~ `0x0109` | KEYDOWN, KEYUP, CHAR |
| 剪贴板 | `0x0300` ~ `0x0304` | CUT, COPY, PASTE, CLEAR, UNDO |
| 用户自定义 | `0x0400` ~ `0x7FFF` | `WM_USER + n` |

### 14.11 一个最简 WndProc 需要处理哪些

```
必处理：
  WM_DESTROY → PostQuitMessage（否则进程退不出）

推荐处理：
  WM_CLOSE   → DestroyWindow（给用户一个优雅关闭的机会）
  WM_PAINT   → BeginPaint/EndPaint（否则窗口一片黑）
  WM_SIZE    → 更新布局

按需处理：
  WM_NCHITTEST   → 自定义 resize/拖拽区域
  WM_MOUSEMOVE   → 悬停效果、坐标跟踪
  WM_LBUTTONDOWN → 点击响应
  WM_KEYDOWN     → 键盘快捷键
  WM_TIMER       → 定时任务
```

---

## 15. WM_PAINT 触发链与绘制流程

### 15.1 核心机制

Windows 不会主动画你的窗口。你告诉它"哪里需要画"，它发 `WM_PAINT`，你在处理函数里画。

```
你调用 InvalidateRect(hwnd, &rect, TRUE)
    ↓
Windows 在窗口的消息队列里标记"这个区域需要重绘"
    ↓
消息循环 GetMessage 取到 WM_PAINT
    ↓
WndProc 收到 WM_PAINT
    ↓
你在 WM_PAINT 中调用 Titlebar_Draw / Widget_Draw
```

### 15.2 InvalidateRect 的含义

```c
InvalidateRect(hwnd, NULL, TRUE);
```

| 参数 | 含义 |
|------|------|
| `hwnd` | 哪个窗口要重绘 |
| `NULL` | 重绘整个客户区（也可以指定一个小矩形） |
| `TRUE` | 擦除背景（画新的之前先把旧的抹掉） |

**InvalidateRect 不等同于立刻画。** 它只是标记"这个地方脏了"，真正的绘制要等到消息循环处理到 `WM_PAINT` 时发生。多个 `InvalidateRect` 调用会被合并成一条 `WM_PAINT`。

### 15.3 标题栏的绘制触发链

| 触发时机 | 谁调用了什么 | 结果 |
|---------|-------------|------|
| 窗口第一次显示 | `ShowWindow` → 系统自动发 WM_PAINT | 画出黄色标题栏 |
| 窗口大小改变 | `WM_SIZE` → `InvalidateRect` | 重绘标题栏（适应新宽度） |
| 鼠标悬停按钮 | `WM_MOUSEMOVE` → `Widget_UpdateHover` → `RedrawTitlebar` | 按钮变色 |
| 鼠标离开窗口 | `WM_MOUSELEAVE` → `RedrawTitlebar` | 按钮恢复 |
| 点击置顶按钮 | `Titlebar_TogglePin` → `RedrawTitlebar` | 按钮蓝底切换 |
| 窗口被遮挡后露出 | 系统自动发 WM_PAINT | 恢复被遮挡的部分 |

### 15.4 WM_PAINT 的标准写法

```c
case WM_PAINT:
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);   // 获取画布，验证无效区域
    // ... 你的所有绘制代码
    EndPaint(hwnd, &ps);               // 结束绘制，标记已重绘
    return 0;
}
```

`BeginPaint` / `EndPaint` 必须成对出现。缺少 `EndPaint` 会导致 `WM_PAINT` 反复触发（Windows 认为还没画完）。

### 15.5 绘制不是"定时执行"的

| 误解 | 真相 |
|------|------|
| "窗口会自己保持画面" | ❌ 窗口被遮挡后内容丢失，必须等 WM_PAINT 才能恢复 |
| "画面在变化时自动更新" | ❌ 必须主动调用 InvalidateRect 触发重绘 |
| "InvalidateRect 立刻画" | ❌ 它只是标记"脏了"，画要等消息循环 |
| "多个 InvalidateRect = 多次 WM_PAINT" | ❌ 合并为一次 |

## 16. 自动靠边隐藏机制

### 16.1 触发链

```
鼠标离开窗口
    ↓
WM_MOUSELEAVE 自动触发（需 TrackMouseEvent 注册）
    ↓
Window_CheckAndAutoHide(hwnd)
    ↓
检查窗口是否在屏幕左右边缘 20px 内
    ↓
是 → SetWindowPos 移到屏幕外，留 3px 色条
    ↓
启动定时器 SetTimer(200ms)
```

### 16.2 弹出链

```
鼠标移动到屏幕边缘
    ↓
定时器 WM_TIMER 每 200ms 检查一次 GetCursorPos
    ↓
鼠标 X 坐标 < 3px + 30px（左边缘）或 > 屏幕宽 - 3px - 30px（右边缘）
    ↓
Window_PopOut(hwnd) → SetWindowPos 滑回原位
    ↓
KillTimer 停止检测
```

### 16.3 两个触发条件的区别

| 触发方式 | 触发时机 | 适合场景 |
|---------|---------|---------|
| `WM_EXITSIZEMOVE`（拖拽结束） | 松开鼠标时立即触发 | 拖到边缘松手隐藏（用户不想要这种） |
| `WM_MOUSELEAVE`（鼠标离开） | 鼠标真正离开窗口才触发 | 窗口在边缘，用户去操作其他窗口时隐藏 ✅ |

### 16.4 隐藏状态管理

```c
enum { HIDE_NONE, HIDE_LEFT, HIDE_RIGHT };
static int s_hideSide = HIDE_NONE;
```

- `HIDE_NONE`：正常显示
- `HIDE_LEFT`：隐藏到左边缘
- `HIDE_RIGHT`：隐藏到右边缘

所有隐藏相关函数（`PopOut`、`Timer`、`ToggleHide`）都检查 `s_hideSide`，确保不会重复操作。

### 16.5 三个入口

| 入口 | 调用的函数 | 效果 |
|------|-----------|------|
| 鼠标离开窗口（自动） | `Window_CheckAndAutoHide` | 边缘检测后自动隐藏 |
| 🗕 按钮点击（手动） | `Window_ToggleHide` | 切换隐藏/显示（默认右边缘） |
| 定时器检测到鼠标靠近（自动） | `Window_PopOut` | 从隐藏中弹出 |
| 拖拽被隐藏的窗口（自动） | `Window_PopOut`（在 OnDragEnd 中） | 拖拽时自动弹出 |

### 16.6 相关系统 API

| API | 作用 |
|-----|------|
| `GetWindowRect` | 获取窗口屏幕坐标 |
| `GetCursorPos` | 获取鼠标屏幕坐标 |
| `SetWindowPos` | 移动窗口位置 |
| `SetTimer` / `KillTimer` | 启动/停止定时器 |
| `GetSystemMetrics(SM_CXSCREEN)` | 获取屏幕宽度 |

## 17. 阶段 2：Rich Edit 文本编辑

### 17.1 Rich Edit 控件简介

Rich Edit 是 Windows 内置的富文本编辑控件，位于 `riched20.dll`。它自带：

| 功能 | 是否需要自己写 |
|------|--------------|
| 打字、删除、光标移动 | ❌ 自带 |
| 文本选中 | ❌ 自带 |
| 复制/粘贴 | ❌ 自带 |
| 撤销/重做（Ctrl+Z/Y） | ❌ 自带 |
| 滚动条 | ❌ 自动出现 |
| Unicode/中文输入 | ❌ 自带 |
| 富文本格式（加粗、颜色） | ❌ 自带，CHARFORMAT2 控制 |
| Markdown 解析 | ✅ 需要自己用 md4c |
| 复选框点击切换 | ✅ 需要自己实现 |

### 17.2 嵌入步骤

```
① LoadLibraryW(L"riched20.dll")  加载系统 DLL
② CreateWindowExW(L"RichEdit20W")  创建控件
③ 调整 WM_SIZE 中控件大小
④ 用 SetWindowTextW / GetWindowTextW 读写内容
```

### 17.3 关键 API

| API | 作用 |
|-----|------|
| `LoadLibraryW(L"riched20.dll")` | 加载 Rich Edit 库 |
| `CreateWindowExW(0, L"RichEdit20W", ...)` | 创建 Rich Edit 控件 |
| `SetWindowTextW(hEdit, text)` | 设置文本内容 |
| `GetWindowTextW(hEdit, buf, len)` | 获取文本内容 |
| `SendMessage(hEdit, EM_SETCHARFORMAT, ...)` | 设置字体、颜色、加粗等格式 |
| `SendMessage(hEdit, EM_GETLINECOUNT, ...)` | 获取总行数 |
| `SendMessage(hEdit, EM_LINEINDEX, i, 0)` | 获取第 i 行的字符索引 |

### 17.4 编辑模式 vs 预览模式

阶段 2 实现两种模式的切换：

| 模式 | 用户看到 | 底层 |
|------|---------|------|
| 编辑模式 | 原始 Markdown 源码 | Rich Edit 纯文本，无格式 |
| 预览模式 | 渲染后的效果（加粗、标题、复选框） | Rich Edit 富文本，有格式 |

切换快捷键：双击标题栏（复用现有逻辑）或 Ctrl+P。

---

*跟随开发阶段同步更新。*
