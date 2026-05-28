cmake_minimum_required(VERSION 3.20)
project(DeskNote VERSION 0.1.0 LANGUAGES C)

# Unicode 编译（Win32 API 宽字符）
add_definitions(-DUNICODE -D_UNICODE)

# ============================================================
# 第三方库：md4c（Markdown 解析器）
# ============================================================
add_library(md4c STATIC
    lib/md4c/src/md4c.c
)
target_include_directories(md4c PUBLIC lib/md4c/src)
target_compile_definitions(md4c PRIVATE MD4C_USE_UTF16)

# ============================================================
# 应用源文件
# ============================================================
set(SOURCES
    src/app/app.c
    src/core/document.c
    src/editor/editor.c
    src/platform/win32/window.c
    src/platform/win32/nonclient.c
    src/render/render.c
    src/storage/note_store.c
    src/storage/state_store.c
    src/storage/storage_write.c
    src/ui/editor_view.c
    src/ui/titlebar.c
    src/ui/button.c
    resources/desknote.rc
)

# 可执行文件（Win32 GUI 程序，无控制台窗口）
add_executable(desknote WIN32 ${SOURCES})

# 链接
target_link_libraries(desknote
    user32
    gdi32
    d2d1
    dwrite
    imm32
)
