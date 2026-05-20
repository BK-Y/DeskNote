# DeskNote 学习笔记 - 基础概念

> 工具链、编译流程、CMake、Win32 窗口模型、关键概念、术语表

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

*跟随开发阶段同步更新。*
