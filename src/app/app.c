#include "app.h"
#include "../editor/editor.h"
#include "../platform/win32/nonclient.h"
#include "../platform/win32/window.h"
#include "../platform/win32/appbar.h"
#include "../render/render.h"
#include "../storage/note_store.h"
#include "../storage/state_store.h"
#include "../ui/editor_view.h"
#include "../ui/titlebar.h"
#include "../ui/button.h"
#include <imm.h>
#include <string.h>
#include <wchar.h>

/* repair-5-a-5: 统一日志前缀（用于贴边修复系列的诊断输出） */
#define R5A_LOG_PREFIX L"[s5a-r5a] "
#define R5A_WriteLog(msg) OutputDebugStringW(msg)

typedef struct {
    HWND hwnd;
    RenderContext* render;
    Editor editor;
    int editor_ready;
    int has_focus;
    int vertical_scroll;
    unsigned int working_version;
    unsigned int persisted_version;
    unsigned int recovery_version;
    int is_dirty;
    DWORD last_edit_tick;
    DWORD last_save_tick;
    DWORD last_recovery_write_tick;
    unsigned int pending_change_count;
    int pending_recovery_write;
    int save_in_progress;
    wchar_t current_file_path[MAX_PATH];
    AppShellState shell;
    ButtonState menu_button_state;       /* 缓存菜单按钮 state */
    int mouse_down;                       /* 鼠标左键按下状态（用于拖拽选择）*/
    int drag_anchor;                      /* 拖拽选择起点偏移 */
} AppState;

typedef struct {
    RenderContext* render;
    const Document* document;
    int vertical_scroll;
    int window_width;
    int window_height;
} AppEditorNavigatorContext;

static AppState g_app;

static void App_UpdateInputCaret(void);
static int App_NavigatorGetVisualPosition(void* context,
                                          int cursor,
                                          EditorVisualPosition* out_position);
static int App_NavigatorHitTestPoint(void* context, int x, int y, int* out_cursor);
static int App_IsRectVisible(RenderRect rect, RenderRect visible_rect);
static DWORD App_GetCurrentTick(void);
static int App_HasElapsedSince(DWORD current_tick, DWORD base_tick, DWORD delay_ms);
static int App_TryLoadRecoverySnapshot(void);
static void App_RecomputeDirty(void);
static void App_ResetSaveState(void);
static void App_RecordTextChange(void);
static void App_RecordRecoverySnapshot(unsigned int version, DWORD current_tick);
static void App_RecordPersistedState(void);
static int App_WriteRecoverySnapshot(DWORD current_tick);
static int App_ClearRecoverySnapshot(void);
static int App_ShouldWriteRecoverySnapshot(DWORD current_tick);
static int App_ShouldAutoSave(DWORD current_tick);
static int App_ResultNeedsRefresh(EditorResult result);
static int App_ApplyShellChromeState(void);

enum {
    APP_AUTOSAVE_IDLE_DELAY_MS = 1500,
    APP_AUTOSAVE_MAX_INTERVAL_MS = 10000,
    APP_AUTOSAVE_PENDING_CHANGE_THRESHOLD = 32,
    APP_RECOVERY_WRITE_DELAY_MS = 1000
};

static int App_SetCurrentFilePath(const wchar_t* path)
{
    size_t path_length;

    if (path == NULL)
        return 1;

    path_length = wcslen(path);
    if (path_length >= MAX_PATH)
        return 1;

    memcpy(g_app.current_file_path, path, (path_length + 1) * sizeof(wchar_t));
    return 0;
}

enum {
    APP_DEFAULT_TITLEBAR_HEIGHT = 32,
    APP_DEFAULT_FRAME_VISUAL_THICKNESS = 1,
    APP_DEFAULT_FRAME_RESIZE_THICKNESS = 6
};

/* repair-5-a-5: 贴边方向钳位（非法值用默认 APP_DOCK_RIGHT） */
static int ClampDockEdge(int edge)
{
    if (edge < APP_DOCK_LEFT || edge > APP_DOCK_BOTTOM)
        return APP_DOCK_RIGHT;
    return edge;
}

/* repair-5-a-5: 贴边厚度钳位（200..500） */
static int ClampDockThickness(int thickness)
{
    if (thickness < 200) return 200;
    if (thickness > 500) return 500;
    return thickness;
}

static void App_InitShellStateDefaults(void)
{
    g_app.shell.use_custom_chrome = 1;
    g_app.shell.titlebar_height = APP_DEFAULT_TITLEBAR_HEIGHT;
    g_app.shell.frame_visual_thickness = APP_DEFAULT_FRAME_VISUAL_THICKNESS;
    g_app.shell.frame_resize_thickness = APP_DEFAULT_FRAME_RESIZE_THICKNESS;
    g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
    g_app.shell.titlebar_command_groups =
        APP_TITLEBAR_COMMAND_GROUP_WINDOW_CONTROLS |
        APP_TITLEBAR_COMMAND_GROUP_SHELL_MODES |
        APP_TITLEBAR_COMMAND_GROUP_BACKGROUND_ACTIONS;
    g_app.shell.active_command = APP_SHELL_COMMAND_NONE;
}

static void App_ApplyShellStateData(const StateData* state)
{
    if (state == NULL)
        return;

    /* 主线产品永久使用 frameless，不从持久化状态读取 use_custom_chrome。
     * 运行态值始终由 App_InitShellStateDefaults 初始化为 1。
     * 底层 Platform_SetFrameless 接口保留供扩展场景使用。 */
    if (state->titlebar_height > 0)
        g_app.shell.titlebar_height = state->titlebar_height;
    if (state->frame_visual_thickness > 0)
        g_app.shell.frame_visual_thickness = state->frame_visual_thickness;

    switch (state->shell_resident_mode)
    {
    case APP_SHELL_RESIDENT_MODE_NONE:
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
        break;

    default:
        break;
    }
}

static void App_WriteShellStateData(StateData* state)
{
    if (state == NULL)
        return;

    state->use_custom_chrome = g_app.shell.use_custom_chrome ? 1 : 0;
    state->titlebar_height = g_app.shell.titlebar_height;
    state->frame_visual_thickness = g_app.shell.frame_visual_thickness;
    state->shell_resident_mode = (int)g_app.shell.resident_mode;
}

static int App_GetShellCommandGroupInternal(AppShellCommand command, AppTitlebarCommandGroup* out_group)
{
    if (out_group == NULL)
        return 1;

    switch (command)
    {
    case APP_SHELL_COMMAND_WINDOW_CLOSE:
    case APP_SHELL_COMMAND_WINDOW_RESTORE:
    case APP_SHELL_COMMAND_SHOW_MENU:
        *out_group = APP_TITLEBAR_COMMAND_GROUP_WINDOW_CONTROLS;
        return 0;

    case APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST:
    case APP_SHELL_COMMAND_ENTER_EDGE_RESERVED:
        *out_group = APP_TITLEBAR_COMMAND_GROUP_SHELL_MODES;
        return 0;

    case APP_SHELL_COMMAND_HIDE_TO_TRAY:
    case APP_SHELL_COMMAND_RESTORE_FROM_TRAY:
    case APP_SHELL_COMMAND_SHOW_TRAY_MENU:
        *out_group = APP_TITLEBAR_COMMAND_GROUP_BACKGROUND_ACTIONS;
        return 0;

    default:
        return 1;
    }
}

static void App_ReportStorageFailure(const wchar_t* message)
{
    if (message == NULL)
        return;

    OutputDebugStringW(message);
}

static int App_LoadInitialDocument(void)
{
    StateData state;
    int state_load_result;
    wchar_t default_path[MAX_PATH];
    wchar_t* loaded_text;
    const wchar_t* target_path;

    if (NoteStore_GetDefaultNotePath(default_path, MAX_PATH) != 0)
        return 1;

    memset(&state, 0, sizeof(state));
    loaded_text = NULL;
    target_path = default_path;

    state_load_result = StateStore_Load(&state);
    if (state_load_result != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to load state.ini, falling back to default note path.\n");
    }
    else
    {
        App_ApplyShellStateData(&state);
    }

    if (state.has_last_file && state.last_file[0] != L'\0')
        target_path = state.last_file;

    if (App_SetCurrentFilePath(target_path) != 0)
        return 1;

    int load_result;

    if (NoteStore_LoadText(g_app.current_file_path, &loaded_text) != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to load note file, starting with empty document.\n");
        if (Editor_SetText(&g_app.editor, L"") == EDITOR_RESULT_ERROR)
            return 1;

        App_ResetSaveState();
        load_result = App_TryLoadRecoverySnapshot();
    }
    else
    {
        if (Editor_SetText(&g_app.editor, loaded_text) == EDITOR_RESULT_ERROR)
        {
            NoteStore_FreeText(loaded_text);
            return 1;
        }

        NoteStore_FreeText(loaded_text);
        App_ResetSaveState();
        load_result = App_TryLoadRecoverySnapshot();
    }

    /* Migration: rewrite state.ini if it had use_custom_chrome != 1.
     * This ensures old state files don't keep a stale use_custom_chrome=0
     * that could confuse developers reading the file. */
    if (state_load_result == 0 && !state.use_custom_chrome)
    {
        StateData write_state;

        memset(&write_state, 0, sizeof(write_state));
        write_state.version = 1;
        write_state.has_last_file = state.has_last_file;
        if (state.has_last_file && state.last_file[0] != L'\0')
            wcsncpy(write_state.last_file, state.last_file, MAX_PATH);
        App_WriteShellStateData(&write_state);
        if (StateStore_Save(&write_state) != 0)
            App_ReportStorageFailure(L"DeskNote: failed to migrate state.ini use_custom_chrome.\n");
    }

    return load_result;
}

static int App_SaveCurrentDocument(void)
{
    StateData state;

    if (!g_app.editor_ready || g_app.current_file_path[0] == L'\0')
        return 1;
    if (g_app.save_in_progress)
        return 1;

    g_app.save_in_progress = 1;

    if (NoteStore_SaveText(g_app.current_file_path,
                           Document_GetText(Editor_GetDocument(&g_app.editor))) != 0)
    {
        g_app.save_in_progress = 0;
        App_ReportStorageFailure(L"DeskNote: failed to save note file.\n");
        return 1;
    }

    memset(&state, 0, sizeof(state));
    state.version = 1;
    state.has_last_file = 1;
    if (wcslen(g_app.current_file_path) >= MAX_PATH)
    {
        g_app.save_in_progress = 0;
        return 1;
    }
    memcpy(state.last_file,
           g_app.current_file_path,
           (wcslen(g_app.current_file_path) + 1) * sizeof(wchar_t));
    App_WriteShellStateData(&state);

    if (StateStore_Save(&state) != 0)
    {
        g_app.save_in_progress = 0;
        App_ReportStorageFailure(L"DeskNote: failed to save state.ini.\n");
        return 1;
    }

    App_RecordPersistedState();
    if (App_ClearRecoverySnapshot() != 0)
        App_ReportStorageFailure(L"DeskNote: failed to delete recovery snapshot after formal save.\n");
    g_app.save_in_progress = 0;
    return 0;
}

static DWORD App_GetCurrentTick(void)
{
    return GetTickCount();
}

static int App_HasElapsedSince(DWORD current_tick, DWORD base_tick, DWORD delay_ms)
{
    return (current_tick - base_tick) >= delay_ms;
}

static void App_RecomputeDirty(void)
{
    g_app.is_dirty = (g_app.working_version != g_app.persisted_version) ? 1 : 0;
}

static int App_TryLoadRecoverySnapshot(void)
{
    NoteRecoverySnapshot snapshot;
    NoteRecoveryLoadResult load_result;
    DWORD current_tick;
    const wchar_t* current_text;

    load_result = NoteStore_LoadRecoverySnapshot(g_app.current_file_path, &snapshot);
    if (load_result == NOTE_RECOVERY_LOAD_NOT_FOUND)
        return 0;
    if (load_result == NOTE_RECOVERY_LOAD_INVALID)
    {
        App_ReportStorageFailure(L"DeskNote: invalid recovery snapshot, deleting it.\n");
        (void)NoteStore_DeleteRecoverySnapshot(g_app.current_file_path);
        return 0;
    }
    if (load_result != NOTE_RECOVERY_LOAD_OK)
    {
        App_ReportStorageFailure(L"DeskNote: failed to load recovery snapshot.\n");
        return 0;
    }

    current_text = Document_GetText(Editor_GetDocument(&g_app.editor));
    if (wcscmp(snapshot.text, current_text) == 0 || snapshot.version <= g_app.persisted_version)
    {
        (void)NoteStore_DeleteRecoverySnapshot(g_app.current_file_path);
        NoteStore_FreeRecoverySnapshot(&snapshot);
        return 0;
    }

    if (Editor_SetText(&g_app.editor, snapshot.text) == EDITOR_RESULT_ERROR)
    {
        NoteStore_FreeRecoverySnapshot(&snapshot);
        return 1;
    }

    current_tick = App_GetCurrentTick();
    g_app.working_version = snapshot.version;
    g_app.recovery_version = snapshot.version;
    g_app.last_edit_tick = current_tick;
    g_app.last_save_tick = current_tick;
    g_app.last_recovery_write_tick = current_tick;
    g_app.pending_change_count = 0;
    g_app.pending_recovery_write = 0;
    App_RecomputeDirty();
    NoteStore_FreeRecoverySnapshot(&snapshot);
    return 0;
}

static void App_ResetSaveState(void)
{
    DWORD current_tick;

    current_tick = App_GetCurrentTick();
    g_app.working_version = 0;
    g_app.persisted_version = 0;
    g_app.recovery_version = 0;
    g_app.last_edit_tick = current_tick;
    g_app.last_save_tick = current_tick;
    g_app.last_recovery_write_tick = current_tick;
    g_app.pending_change_count = 0;
    g_app.pending_recovery_write = 0;
    g_app.save_in_progress = 0;
    App_RecomputeDirty();
}

static void App_RecordTextChange(void)
{
    g_app.working_version += 1;
    g_app.last_edit_tick = App_GetCurrentTick();
    if (g_app.pending_change_count < 0xffffffffu)
        g_app.pending_change_count += 1;
    g_app.pending_recovery_write = 1;
    App_RecomputeDirty();
}

static void App_RecordRecoverySnapshot(unsigned int version, DWORD current_tick)
{
    g_app.recovery_version = version;
    g_app.last_recovery_write_tick = current_tick;
    g_app.pending_recovery_write = 0;
}

static void App_RecordPersistedState(void)
{
    g_app.persisted_version = g_app.working_version;
    g_app.recovery_version = 0;
    g_app.last_save_tick = App_GetCurrentTick();
    g_app.pending_change_count = 0;
    g_app.pending_recovery_write = 0;
    App_RecomputeDirty();
}

static int App_WriteRecoverySnapshot(DWORD current_tick)
{
    if (!g_app.editor_ready || g_app.current_file_path[0] == L'\0' || !g_app.is_dirty)
        return 1;

    if (NoteStore_SaveRecoverySnapshot(g_app.current_file_path,
                                       g_app.working_version,
                                       Document_GetText(Editor_GetDocument(&g_app.editor))) != 0)
    {
        g_app.last_recovery_write_tick = current_tick;
        return 1;
    }

    App_RecordRecoverySnapshot(g_app.working_version, current_tick);
    return 0;
}

static int App_ClearRecoverySnapshot(void)
{
    if (g_app.current_file_path[0] == L'\0')
        return 1;
    if (NoteStore_DeleteRecoverySnapshot(g_app.current_file_path) != 0)
        return 1;
    g_app.recovery_version = 0;
    g_app.pending_recovery_write = 0;
    return 0;
}

static int App_ShouldWriteRecoverySnapshot(DWORD current_tick)
{
    if (!g_app.editor_ready || g_app.current_file_path[0] == L'\0')
        return 0;
    if (!g_app.is_dirty || g_app.save_in_progress || !g_app.pending_recovery_write)
        return 0;
    if (g_app.recovery_version >= g_app.working_version)
        return 0;
    return App_HasElapsedSince(current_tick,
                               g_app.last_recovery_write_tick,
                               APP_RECOVERY_WRITE_DELAY_MS);
}

static int App_ShouldAutoSave(DWORD current_tick)
{
    if (!g_app.editor_ready || g_app.current_file_path[0] == L'\0')
        return 0;
    if (!g_app.is_dirty || g_app.save_in_progress)
        return 0;
    if (App_HasElapsedSince(current_tick, g_app.last_edit_tick, APP_AUTOSAVE_IDLE_DELAY_MS))
        return 1;
    if (App_HasElapsedSince(current_tick, g_app.last_save_tick, APP_AUTOSAVE_MAX_INTERVAL_MS))
        return 1;
    if (g_app.pending_change_count >= APP_AUTOSAVE_PENDING_CHANGE_THRESHOLD)
        return 1;
    return 0;
}

static int App_ResultNeedsRefresh(EditorResult result)
{
    return result == EDITOR_RESULT_CURSOR_MOVED || result == EDITOR_RESULT_TEXT_CHANGED;
}

static int App_GetClientSize(unsigned int* width, unsigned int* height)
{
    RECT client_rect;

    if (g_app.hwnd == NULL || width == NULL || height == NULL)
        return 1;
    if (!GetClientRect(g_app.hwnd, &client_rect))
        return 1;

    *width = (unsigned int)(client_rect.right - client_rect.left);
    *height = (unsigned int)(client_rect.bottom - client_rect.top);
    return 0;
}

static int App_GetMaxVerticalScroll(int* out_scroll)
{
    unsigned int width;
    unsigned int height;
    RenderRect text_rect;
    int content_height;
    int max_scroll;

    if (out_scroll == NULL || !g_app.editor_ready || g_app.render == NULL)
        return 1;
    if (App_GetClientSize(&width, &height) != 0)
        return 1;
    if (EditorView_GetTextRect((int)width, (int)height, &text_rect) != 0)
        return 1;
    if (EditorView_GetContentHeight(g_app.render,
                                    Editor_GetDocument(&g_app.editor),
                                    (int)width,
                                    (int)height,
                                    &content_height) != 0)
    {
        return 1;
    }

    max_scroll = content_height - text_rect.height;
    if (max_scroll < 0)
        max_scroll = 0;
    *out_scroll = max_scroll;
    return 0;
}

static void App_ClampVerticalScroll(void)
{
    int max_scroll;

    if (g_app.vertical_scroll < 0)
        g_app.vertical_scroll = 0;
    if (App_GetMaxVerticalScroll(&max_scroll) != 0)
    {
        if (g_app.vertical_scroll < 0)
            g_app.vertical_scroll = 0;
        return;
    }
    if (g_app.vertical_scroll > max_scroll)
        g_app.vertical_scroll = max_scroll;
}

static void App_RequestRefresh(void)
{
    if (g_app.hwnd == NULL)
        return;

    App_UpdateInputCaret();
    InvalidateRect(g_app.hwnd, NULL, FALSE);
}

static int App_NavigatorGetVisualPosition(void* context,
                                          int cursor,
                                          EditorVisualPosition* out_position)
{
    AppEditorNavigatorContext* navigator_context;

    navigator_context = (AppEditorNavigatorContext*)context;
    if (navigator_context == NULL || out_position == NULL)
        return 1;

    return EditorView_GetCursorVisualPosition(navigator_context->render,
                                              navigator_context->document,
                                              cursor,
                                              navigator_context->vertical_scroll,
                                              navigator_context->window_width,
                                              navigator_context->window_height,
                                              out_position);
}

static int App_NavigatorHitTestPoint(void* context, int x, int y, int* out_cursor)
{
    AppEditorNavigatorContext* navigator_context;
    RenderRect text_rect;

    navigator_context = (AppEditorNavigatorContext*)context;
    if (navigator_context == NULL || out_cursor == NULL)
        return 1;
    if (EditorView_GetTextRect(navigator_context->window_width,
                               navigator_context->window_height,
                               &text_rect) != 0)
    {
        return 1;
    }

    if (text_rect.width > 0)
    {
        if (x < text_rect.x)
            x = text_rect.x;
        if (x > text_rect.x + text_rect.width - 1)
            x = text_rect.x + text_rect.width - 1;
    }
    if (text_rect.height > 0)
    {
        if (y < text_rect.y)
            y = text_rect.y;
        if (y > text_rect.y + text_rect.height - 1)
            y = text_rect.y + text_rect.height - 1;
    }

    return EditorView_HitTestCursor(navigator_context->render,
                                    navigator_context->document,
                                    navigator_context->window_width,
                                    navigator_context->window_height,
                                    navigator_context->vertical_scroll,
                                    x,
                                    y,
                                    out_cursor);
}

static int App_IsRectVisible(RenderRect rect, RenderRect visible_rect)
{
    int rect_right;
    int rect_bottom;
    int visible_right;
    int visible_bottom;

    rect_right = rect.x + rect.width;
    rect_bottom = rect.y + rect.height;
    visible_right = visible_rect.x + visible_rect.width;
    visible_bottom = visible_rect.y + visible_rect.height;

    if (rect.width <= 0 || rect.height <= 0 || visible_rect.width <= 0 || visible_rect.height <= 0)
        return 0;

    return rect.x < visible_right &&
           rect_right > visible_rect.x &&
           rect.y < visible_bottom &&
           rect_bottom > visible_rect.y;
}

static int App_MoveCursorVertical(int direction)
{
    AppEditorNavigatorContext navigator_context;
    EditorVerticalNavigator navigator;
    unsigned int width;
    unsigned int height;

    if (!g_app.editor_ready || g_app.render == NULL)
        return 1;
    if (App_GetClientSize(&width, &height) != 0)
        return 1;

    navigator_context.render = g_app.render;
    navigator_context.document = Editor_GetDocument(&g_app.editor);
    navigator_context.vertical_scroll = g_app.vertical_scroll;
    navigator_context.window_width = (int)width;
    navigator_context.window_height = (int)height;

    navigator.context = &navigator_context;
    navigator.get_visual_position = App_NavigatorGetVisualPosition;
    navigator.hit_test_point = App_NavigatorHitTestPoint;
    return Editor_MoveCursorVertical(&g_app.editor, &navigator, direction);
}

static void App_UpdateImeWindow(RenderRect caret_rect)
{
    HIMC input_context;
    COMPOSITIONFORM composition_form;
    CANDIDATEFORM candidate_form;

    if (g_app.hwnd == NULL)
        return;

    input_context = ImmGetContext(g_app.hwnd);
    if (input_context == NULL)
        return;

    ZeroMemory(&composition_form, sizeof(composition_form));
    composition_form.dwStyle = CFS_FORCE_POSITION;
    composition_form.ptCurrentPos.x = caret_rect.x;
    composition_form.ptCurrentPos.y = caret_rect.y + caret_rect.height;
    ImmSetCompositionWindow(input_context, &composition_form);

    ZeroMemory(&candidate_form, sizeof(candidate_form));
    candidate_form.dwIndex = 0;
    candidate_form.dwStyle = CFS_CANDIDATEPOS;
    candidate_form.ptCurrentPos.x = caret_rect.x;
    candidate_form.ptCurrentPos.y = caret_rect.y + caret_rect.height;
    ImmSetCandidateWindow(input_context, &candidate_form);

    ImmReleaseContext(g_app.hwnd, input_context);
}

static void App_UpdateInputCaret(void)
{
    RenderRect caret_rect;
    RenderRect text_rect;
    unsigned int width;
    unsigned int height;

    if (!g_app.editor_ready || !g_app.has_focus || g_app.hwnd == NULL || g_app.render == NULL)
        return;
    if (App_GetClientSize(&width, &height) != 0)
        return;
    if (EditorView_GetTextRect((int)width, (int)height, &text_rect) != 0)
        return;
    if (EditorView_GetCaretRect(g_app.render,
                                Editor_GetDocument(&g_app.editor),
                                Editor_GetCursor(&g_app.editor),
                                g_app.vertical_scroll,
                                (int)width,
                                (int)height,
                                &caret_rect) != 0)
        return;
    if (!App_IsRectVisible(caret_rect, text_rect))
    {
        DestroyCaret();
        return;
    }

    DestroyCaret();
    if (!CreateCaret(g_app.hwnd, NULL, caret_rect.width, caret_rect.height))
        return;

    SetCaretPos(caret_rect.x, caret_rect.y);
    ShowCaret(g_app.hwnd);
    App_UpdateImeWindow(caret_rect);
}

static void App_EnsureCaretVisible(void)
{
    RenderRect caret_rect;
    RenderRect text_rect;
    unsigned int width;
    unsigned int height;
    int caret_bottom;
    int visible_bottom;

    if (!g_app.editor_ready || g_app.render == NULL)
        return;
    if (App_GetClientSize(&width, &height) != 0)
        return;
    if (EditorView_GetTextRect((int)width, (int)height, &text_rect) != 0)
        return;
    if (EditorView_GetCaretRect(g_app.render,
                                Editor_GetDocument(&g_app.editor),
                                Editor_GetCursor(&g_app.editor),
                                g_app.vertical_scroll,
                                (int)width,
                                (int)height,
                                &caret_rect) != 0)
    {
        return;
    }

    if (caret_rect.y < text_rect.y)
        g_app.vertical_scroll -= (text_rect.y - caret_rect.y);
    else
    {
        caret_bottom = caret_rect.y + caret_rect.height;
        visible_bottom = text_rect.y + text_rect.height;
        if (caret_bottom > visible_bottom)
            g_app.vertical_scroll += (caret_bottom - visible_bottom);
    }

    App_ClampVerticalScroll();
}

/* repair-5-a-1: 从持久化状态读取配置并尝试注册 AppBar（仅 EDGE_RESERVED 模式） */
static void App_TryRegisterAppBarFromState(HWND hwnd)
{
    StateData state;
    StateStore_Load(&state);

    if (state.shell_resident_mode != APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
        return;

    AppDockEdge edge = (AppDockEdge)ClampDockEdge(state.dock_edge);
    int thickness = ClampDockThickness(state.dock_thickness);

    wchar_t buf[256];
    swprintf(buf, 256, R5A_LOG_PREFIX L"App_TryRegisterAppBarFromState: edge=%d thickness=%d\n",
             (int)edge, thickness);
    R5A_WriteLog(buf);

    if (AppBar_Register(hwnd) == 0)
    {
        AppBar_SetPosition(hwnd, edge, thickness);
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;
        R5A_WriteLog(R5A_LOG_PREFIX L"App_TryRegisterAppBarFromState: success\n");
    }
    else
    {
        R5A_WriteLog(R5A_LOG_PREFIX L"App_TryRegisterAppBarFromState: AppBar_Register failed, state preserved\n");
        /* 失败保留 state，由 5b 重试 */
    }
}

int App_Run(void)
{
    return Window_Run();
}

int App_Init(HWND hwnd)
{
    if (hwnd == NULL)
        return 1;

    g_app.hwnd = hwnd;
    g_app.render = Render_Create();
    if (g_app.render == NULL)
        return 1;

    if (Render_Init(g_app.render, hwnd) != 0)
    {
        Render_Destroy(g_app.render);
        g_app.render = NULL;
        return 1;
    }

    if (Editor_Init(&g_app.editor) != 0)
    {
        Render_Destroy(g_app.render);
        g_app.render = NULL;
        return 1;
    }

    g_app.editor_ready = 1;
    g_app.has_focus = 0;
    g_app.vertical_scroll = 0;
    g_app.working_version = 0;
    g_app.persisted_version = 0;
    g_app.recovery_version = 0;
    g_app.is_dirty = 0;
    g_app.last_edit_tick = 0;
    g_app.last_save_tick = 0;
    g_app.last_recovery_write_tick = 0;
    g_app.pending_change_count = 0;
    g_app.pending_recovery_write = 0;
    g_app.save_in_progress = 0;
    g_app.current_file_path[0] = L'\0';
    g_app.menu_button_state = BUTTON_STATE_NORMAL;
    g_app.mouse_down = 0;
    App_InitShellStateDefaults();

    if (App_LoadInitialDocument() != 0)
    {
        Editor_Free(&g_app.editor);
        g_app.editor_ready = 0;
        Render_Destroy(g_app.render);
        g_app.render = NULL;
        return 1;
    }

    if (App_ApplyShellChromeState() != 0)
    {
        Editor_Free(&g_app.editor);
        g_app.editor_ready = 0;
        Render_Destroy(g_app.render);
        g_app.render = NULL;
        return 1;
    }

    /* Shell-3c_2: 启动时恢复上次的 presence 状态 */
    {
        StateData state;
        StateStore_Load(&state);
        if (state.presence_state == 1) /* hidden_to_tray */
        {
            ShowWindow(hwnd, SW_HIDE);
        }
    }

    /* Shell-4a_2: 启动时恢复浮动置顶状态 */
    {
        StateData state;
        StateStore_Load(&state);
        if (state.shell_resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        {
            SetWindowPos(hwnd, HWND_TOPMOST,
                        state.last_floating_left,
                        state.last_floating_top,
                        state.last_floating_width,
                        state.last_floating_height,
                        SWP_NOZORDER);
        }
    }

    /* repair-5-a-1: 启动时尝试恢复贴边占位 */
    App_TryRegisterAppBarFromState(hwnd);

    App_EnsureCaretVisible();

    return 0;
}

void App_Shutdown(void)
{
    if (g_app.editor_ready)
    {
        if (App_SaveCurrentDocument() != 0 && g_app.is_dirty)
        {
            if (App_WriteRecoverySnapshot(App_GetCurrentTick()) != 0)
                App_ReportStorageFailure(L"DeskNote: failed to write recovery snapshot during shutdown.\n");
        }
        Editor_Free(&g_app.editor);
        g_app.editor_ready = 0;
    }

    DestroyCaret();
    g_app.has_focus = 0;
    g_app.vertical_scroll = 0;
    g_app.working_version = 0;
    g_app.persisted_version = 0;
    g_app.recovery_version = 0;
    g_app.is_dirty = 0;
    g_app.last_edit_tick = 0;
    g_app.last_save_tick = 0;
    g_app.last_recovery_write_tick = 0;
    g_app.pending_change_count = 0;
    g_app.pending_recovery_write = 0;
    g_app.save_in_progress = 0;
    App_InitShellStateDefaults();

    if (g_app.render != NULL)
    {
        Render_Destroy(g_app.render);
        g_app.render = NULL;
    }

    g_app.hwnd = NULL;
    g_app.current_file_path[0] = L'\0';
}

int App_OnResize(unsigned int width, unsigned int height)
{
    if (g_app.render == NULL)
        return 1;

    if (Render_Resize(g_app.render, width, height) != 0)
        return 1;

    return Platform_RebuildClientArea(g_app.hwnd);
}

void App_OnPaint(void)
{
    unsigned int width;
    unsigned int height;

    if (g_app.hwnd == NULL || g_app.render == NULL)
        return;
    if (App_GetClientSize(&width, &height) != 0)
        return;

    if (Render_BeginFrame(g_app.render) != 0)
        return;

    Render_Clear(g_app.render, (RenderColor){ 246, 246, 246, 255 });

    /* 绘制标题栏和窗口边框 */
    {
        TitlebarLayout titlebar_layout;

        titlebar_layout = Titlebar_CalculateLayout(
            (int)width, (int)height,
            g_app.shell.titlebar_height,
            g_app.shell.frame_visual_thickness,
            g_app.shell.titlebar_command_groups);

        /* 应用缓存的 button state */
        titlebar_layout.menu_button.state = g_app.menu_button_state;

        /* Shell-5d: 根据 resident_mode 更新指示灯外观 */
        Titlebar_UpdateStatus(&titlebar_layout.status_indicator, g_app.shell.resident_mode);

        Titlebar_Draw(g_app.render, &titlebar_layout);
    }

    EditorView_Draw(g_app.render,
                    Editor_GetDocument(&g_app.editor),
                    Editor_GetCursor(&g_app.editor),
                    g_app.vertical_scroll,
                    (int)width,
                    (int)height,
                    Editor_GetSelectionAnchor(&g_app.editor),
                    Editor_GetSelectionActive(&g_app.editor),
                    Editor_HasSelection(&g_app.editor));
    (void)Render_EndFrame(g_app.render);
    App_UpdateInputCaret();
}

void App_OnMouseMove(int x, int y)
{
    unsigned int tmp_width, tmp_height;

    if (g_app.hwnd == NULL || g_app.render == NULL)
        return;

    /* 更新缓存 state：按钮在窗口右上角 [width-46, width) */
    if (App_GetClientSize(&tmp_width, &tmp_height) == 0) {
        int btn_right = (int)tmp_width;
        int btn_left  = (int)tmp_width - 46;
        if (x >= btn_left && x < btn_right &&
            y >= g_app.shell.frame_visual_thickness &&
            y < g_app.shell.frame_visual_thickness + g_app.shell.titlebar_height)
            g_app.menu_button_state = BUTTON_STATE_HOVER;
        else
            g_app.menu_button_state = BUTTON_STATE_NORMAL;
    }
    
    /* 拖拽选择：鼠标按下时在编辑区移动 */
    if (g_app.mouse_down && g_app.editor_ready)
    {
        unsigned int sel_w, sel_h;
        int sel_cursor;
        if (App_GetClientSize(&sel_w, &sel_h) == 0 &&
            EditorView_HitTestCursor(g_app.render,
                                     Editor_GetDocument(&g_app.editor),
                                     (int)sel_w,
                                     (int)sel_h,
                                     g_app.vertical_scroll,
                                     x, y,
                                     &sel_cursor) == 0)
        {
            Editor_SetSelection(&g_app.editor, g_app.drag_anchor, sel_cursor);
        }
    }

    App_RequestRefresh();
}

void App_OnMouseLeave(void)
{
    g_app.menu_button_state = BUTTON_STATE_NORMAL;
    App_RequestRefresh();
}

void App_OnChar(wchar_t ch)
{
    EditorResult result;

    if (!g_app.editor_ready || g_app.hwnd == NULL)
        return;

    result = EDITOR_RESULT_UNCHANGED;
    if (ch == L'\r')
        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_ENTER);
    else if (ch >= L' ')
        result = Editor_HandleChar(&g_app.editor, ch);

    if (result == EDITOR_RESULT_TEXT_CHANGED)
        App_RecordTextChange();

    if (App_ResultNeedsRefresh(result))
    {
        App_EnsureCaretVisible();
        App_RequestRefresh();
    }
}

void App_OnKeyDown(unsigned int key)
{
    EditorResult result;

    if (!g_app.editor_ready || g_app.hwnd == NULL)
        return;

    result = EDITOR_RESULT_UNCHANGED;

    /* Ctrl+C/V/X — 在 switch(VK_*) 之前处理，因为它们是字符键 */
    if (GetKeyState(VK_CONTROL) < 0)
    {
        switch (key)
        {
        case 'C':
        {
            if (Editor_HasSelection(&g_app.editor))
            {
                int s = Editor_GetSelectionAnchor(&g_app.editor);
                int e = Editor_GetSelectionActive(&g_app.editor);
                if (s > e) { int t = s; s = e; e = t; }
                int len = e - s;
                if (len > 0)
                {
                    const wchar_t* txt = Document_GetText(Editor_GetDocument(&g_app.editor));
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
                    if (hMem)
                    {
                        wchar_t* dst = GlobalLock(hMem);
                        wcsncpy(dst, txt + s, len);
                        dst[len] = L'\0';
                        GlobalUnlock(hMem);
                        if (OpenClipboard(g_app.hwnd))
                        {
                            EmptyClipboard();
                            SetClipboardData(CF_UNICODETEXT, hMem);
                            CloseClipboard();
                        }
                        else
                            GlobalFree(hMem);
                    }
                }
            }
            return;
        }
        case 'X':
        {
            int s = Editor_GetSelectionAnchor(&g_app.editor);
            int e = Editor_GetSelectionActive(&g_app.editor);
            if (s > e) { int t = s; s = e; e = t; }
            int len = e - s;
            if (len > 0 && Editor_HasSelection(&g_app.editor))
            {
                const wchar_t* txt = Document_GetText(Editor_GetDocument(&g_app.editor));
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
                if (hMem)
                {
                    wchar_t* dst = GlobalLock(hMem);
                    wcsncpy(dst, txt + s, len);
                    dst[len] = L'\0';
                    GlobalUnlock(hMem);
                    if (OpenClipboard(g_app.hwnd))
                    {
                        EmptyClipboard();
                        SetClipboardData(CF_UNICODETEXT, hMem);
                        CloseClipboard();
                        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_BACKSPACE);
                    }
                    else
                        GlobalFree(hMem);
                }
            }
            break;
        }
        case 'V':
        {
            if (OpenClipboard(g_app.hwnd))
            {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData)
                {
                    const wchar_t* clip = (const wchar_t*)GlobalLock(hData);
                    if (clip)
                    {
                        result = Editor_InsertText(&g_app.editor, clip);
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            break;
        }
        }
    }

    switch (key)
    {
    case VK_LEFT:
        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_LEFT);
        break;

    case VK_RIGHT:
        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_RIGHT);
        break;

    case VK_UP:
        result = App_MoveCursorVertical(-1);
        break;

    case VK_DOWN:
        result = App_MoveCursorVertical(1);
        break;

    case VK_BACK:
        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_BACKSPACE);
        break;

    case VK_DELETE:
        result = Editor_HandleKey(&g_app.editor, EDITOR_KEY_DELETE);
        break;
    }

    if (result == EDITOR_RESULT_TEXT_CHANGED)
        App_RecordTextChange();

    if (App_ResultNeedsRefresh(result))
    {
        App_EnsureCaretVisible();
        App_RequestRefresh();
    }
}

void App_OnLeftButtonDown(int x, int y)
{
    unsigned int width;
    unsigned int height;
    TitlebarLayout layout;
    TitlebarHitResult hit;
    int cursor;
    EditorResult result;

    if (!g_app.editor_ready || g_app.hwnd == NULL || g_app.render == NULL)
        return;
    if (App_GetClientSize(&width, &height) != 0)
        return;

    /* 1. Titlebar hit test: menu button or drag */
    layout = Titlebar_CalculateLayout(
        (int)width, (int)height,
        g_app.shell.titlebar_height,
        g_app.shell.frame_visual_thickness,
        g_app.shell.titlebar_command_groups);
    hit = Titlebar_HitTest(&layout, x, y);
    if (hit == TITLEBAR_HIT_MENU_BUTTON) {
        g_app.menu_button_state = BUTTON_STATE_PRESSED;
        App_SubmitShellCommand(APP_SHELL_COMMAND_SHOW_MENU);
        return;
    }
    if (hit == TITLEBAR_HIT_DRAG_BAR) {
        g_app.menu_button_state = BUTTON_STATE_NORMAL;
        Platform_BeginWindowDrag(g_app.hwnd);
    }

    /* 2. Otherwise: editor cursor positioning */
    if (EditorView_HitTestCursor(g_app.render,
                                 Editor_GetDocument(&g_app.editor),
                                 (int)width,
                                 (int)height,
                                 g_app.vertical_scroll,
                                 x,
                                 y,
                                 &cursor) != 0)
    {
        return;
    }
    result = Editor_SetCursor(&g_app.editor, cursor);
    if (result == EDITOR_RESULT_ERROR)
        return;

    /* 单击清除选区 */
    if (Editor_HasSelection(&g_app.editor))
        Editor_ClearSelection(&g_app.editor);

    if (App_ResultNeedsRefresh(result))
    {
        g_app.mouse_down = 1;
        g_app.drag_anchor = cursor;
        App_EnsureCaretVisible();
        App_RequestRefresh();
    }
}

/* 鼠标左键释放 — 结束拖拽选择 */
void App_OnLeftButtonUp(int x, int y)
{
    (void)x;
    (void)y;
    g_app.mouse_down = 0;
}

/* 鼠标左键双击 — 选中当前词 */
void App_OnLeftButtonDoubleClick(int x, int y)
{
    unsigned int dbl_width, dbl_height;
    int dbl_cursor;

    if (!g_app.editor_ready || g_app.hwnd == NULL || g_app.render == NULL)
        return;
    if (App_GetClientSize(&dbl_width, &dbl_height) != 0)
        return;

    if (EditorView_HitTestCursor(g_app.render,
                                 Editor_GetDocument(&g_app.editor),
                                 (int)dbl_width,
                                 (int)dbl_height,
                                 g_app.vertical_scroll,
                                 x, y,
                                 &dbl_cursor) != 0)
    {
        return;
    }

    Editor_SetCursor(&g_app.editor, dbl_cursor);
    {
        int wstart, wend;
        Editor_GetWordBoundary(&g_app.editor, &wstart, &wend);
        if (wstart != wend)
            Editor_SetSelection(&g_app.editor, wstart, wend);
    }
    g_app.mouse_down = 0;
    App_EnsureCaretVisible();
    App_RequestRefresh();
}

void App_OnMouseWheel(int delta)
{
    const int scroll_step = 48;
    int lines;
    int old_scroll;

    if (!g_app.editor_ready)
        return;

    lines = delta / WHEEL_DELTA;
    if (lines == 0)
        lines = delta > 0 ? 1 : -1;

    old_scroll = g_app.vertical_scroll;
    g_app.vertical_scroll -= lines * scroll_step;
    App_ClampVerticalScroll();
    if (g_app.vertical_scroll != old_scroll)
        App_RequestRefresh();
}

void App_OnTick(void)
{
    DWORD current_tick;

    current_tick = App_GetCurrentTick();
    if (App_ShouldWriteRecoverySnapshot(current_tick))
    {
        if (App_WriteRecoverySnapshot(current_tick) != 0)
            App_ReportStorageFailure(L"DeskNote: failed to write recovery snapshot.\n");
    }
    if (!App_ShouldAutoSave(current_tick))
        return;

    (void)App_SaveCurrentDocument();
}

void App_OnFocusGained(void)
{
    g_app.has_focus = 1;
    App_UpdateInputCaret();
}

void App_OnFocusLost(void)
{
    g_app.has_focus = 0;
    DestroyCaret();
}

void App_OnImeStartComposition(void)
{
    App_UpdateInputCaret();
}

int App_GetSaveState(AppSaveState* out_state)
{
    if (out_state == NULL)
        return 1;

    out_state->working_version = g_app.working_version;
    out_state->persisted_version = g_app.persisted_version;
    out_state->recovery_version = g_app.recovery_version;
    out_state->is_dirty = g_app.is_dirty;
    return 0;
}

int App_IsDirty(void)
{
    return g_app.is_dirty;
}

int App_GetShellState(AppShellState* out_state)
{
    if (out_state == NULL)
        return 1;

    *out_state = g_app.shell;
    return 0;
}

int App_SubmitShellCommand(AppShellCommand command)
{
    AppTitlebarCommandGroup group;

    if (command == APP_SHELL_COMMAND_NONE)
        return 1;

    /* SHOW_MENU requires immediate execution (TrackPopupMenu) */
    if (command == APP_SHELL_COMMAND_SHOW_MENU)
    {
        HMENU hMenu;
        int cmd;
        POINT pt;

        GetCursorPos(&pt);
        hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 100, L"隐藏");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING |
            (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST ? MF_CHECKED : 0),
            102, L"浮动置顶");
        AppendMenuW(hMenu, MF_STRING |
            (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED ? MF_CHECKED : 0),
            103, L"贴边占位");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 101, L"关于 DeskNote...");
        cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                             pt.x, pt.y, 0, g_app.hwnd, NULL);
        DestroyMenu(hMenu);

        if (cmd == 100) /* 隐藏到托盘 */
            App_HideToTray(g_app.hwnd);
        else if (cmd == 102) /* 浮动置顶 */
        {
            /* Shell-4a_2: 先保存当前窗口位置到 state.ini */
            RECT rect;
            if (GetWindowRect(g_app.hwnd, &rect))
            {
                StateData state;
                StateStore_Load(&state);
                state.last_floating_left   = rect.left;
                state.last_floating_top    = rect.top;
                state.last_floating_width  = rect.right - rect.left;
                state.last_floating_height = rect.bottom - rect.top;
                StateStore_Save(&state);
            }
            /* 提交切换命令，由 window.c 收束执行 SetWindowPos */
            App_SubmitShellCommand(APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST);
        }
        else if (cmd == 103) /* 贴边占位 */
        {
            /* Shell-5a: 先更新 resident_mode 并保存 dock 配置 */
            if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
            {
                g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
            }
            else
            {
                /* repair-5-a-3: 从浮动置顶切到贴边时，先保存 last_floating_* 并取消 topmost */
                if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
                {
                    RECT rect;
                    if (GetWindowRect(g_app.hwnd, &rect))
                    {
                        StateData st;
                        StateStore_Load(&st);
                        st.last_floating_left   = rect.left;
                        st.last_floating_top    = rect.top;
                        st.last_floating_width  = rect.right - rect.left;
                        st.last_floating_height = rect.bottom - rect.top;
                        StateStore_Save(&st);
                        R5A_WriteLog(R5A_LOG_PREFIX L"cmd103: saved last_floating_* from topmost mode\n");
                    }

                    SetWindowPos(g_app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    R5A_WriteLog(R5A_LOG_PREFIX L"cmd103: cancelled topmost before edge_reserved\n");
                }

                g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_EDGE_RESERVED;

                /* 保存 dock 配置到 state.ini */
                StateData state;
                StateStore_Load(&state);
                state.dock_edge = APP_DOCK_RIGHT;
                state.dock_thickness = 240;
                StateStore_Save(&state);
            }

            /* 同步执行 AppBar 注册/注销，不经过异步命令队列 */
            HWND hwnd = g_app.hwnd;
            if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
            {
                if (!AppBar_IsRegistered(hwnd))
                {
                    OutputDebugStringW(R5A_LOG_PREFIX L"cmd103: registering AppBar (sync)\n");
                    if (AppBar_Register(hwnd) == 0)
                        AppBar_SetPosition(hwnd, APP_DOCK_RIGHT, 240);
                }
            }
            else
            {
                if (AppBar_IsRegistered(hwnd))
                {
                    OutputDebugStringW(R5A_LOG_PREFIX L"cmd103: unregistering AppBar (sync)\n");
                    AppBar_Unregister(hwnd);
                }
                /* 退出贴边后把窗口移离边缘，避免下次注册时位置冲突 */
                {
                    RECT rc;
                    GetWindowRect(hwnd, &rc);
                    int w = rc.right - rc.left;
                    int h = rc.bottom - rc.top;
                    int x = rc.left;
                    int y = rc.top;
                    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
                    int cw = rc.right - rc.left;
                    int ch = rc.bottom - rc.top;
                    /* 如果窗口在右边缘或右下角，移到屏幕中央 */
                    if (x + w >= rc.right - 10)
                        x = (cw - w) / 2;
                    if (y + h >= rc.bottom - 10)
                        y = (ch - h) / 2;
                    MoveWindow(hwnd, x, y, w, h, TRUE);
                }
            }
            App_UpdateTrayTip(hwnd);  /* Shell-5c: 提示文字同步 resident_mode */
        }
        else if (cmd == 101) /* 关于 */
            MessageBoxW(g_app.hwnd,
                L"DeskNote v0.1\n\n"
                L"A lightweight desktop notes application.\n\n"
                L"Uses md4c - Markdown for C\n"
                L"Copyright (c) 2016, Martin Mitas\n"
                L"MIT License",
                L"关于 DeskNote",
                MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    /* Other commands: validate group and store */
    if (App_GetShellCommandGroupInternal(command, &group) != 0)
        return 1;
    if ((g_app.shell.titlebar_command_groups & (unsigned int)group) == 0)
        return 1;

    g_app.shell.active_command = command;
    return 0;
}

int App_TakePendingShellCommand(AppShellCommand* out_command)
{
    if (out_command == NULL)
        return 1;
    if (g_app.shell.active_command == APP_SHELL_COMMAND_NONE)
        return 1;

    *out_command = g_app.shell.active_command;
    g_app.shell.active_command = APP_SHELL_COMMAND_NONE;
    return 0;
}

int App_GetShellCommandGroup(AppShellCommand command, AppTitlebarCommandGroup* out_group)
{
    return App_GetShellCommandGroupInternal(command, out_group);
}

int App_EnableCustomChrome(int enable)
{
    StateData state;
    int result = 0;

    g_app.shell.use_custom_chrome = enable ? 1 : 0;

    /* Read existing state if available, then write shell fields and persist */
    memset(&state, 0, sizeof(state));
    (void)StateStore_Load(&state);
    App_WriteShellStateData(&state);
    if (StateStore_Save(&state) != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to save state.ini when toggling custom chrome.\n");
        result = 1;
    }

    /* Apply platform change if window exists */
    if (g_app.hwnd != NULL)
    {
        if (App_ApplyShellChromeState() != 0)
            result = 1;
    }

    return result ? 1 : 0;
}

int App_RequestClientRebuild(void)
{
    RECT rect;
    unsigned int width;
    unsigned int height;

    if (g_app.hwnd == NULL)
        return 1;

    if (Platform_ComputeContentRect(g_app.hwnd, &rect) != 0)
        return 1;

    width = (unsigned int)(rect.right - rect.left);
    height = (unsigned int)(rect.bottom - rect.top);

    EditorView_OnClientAreaChanged((int)rect.left, (int)rect.top, (int)width, (int)height);
    App_ClampVerticalScroll();
    App_EnsureCaretVisible();
    App_UpdateInputCaret();
    App_RequestRefresh();
    return 0;
}

static int App_ApplyShellChromeState(void)
{
    if (g_app.hwnd == NULL)
        return 1;
    if (Platform_Nonclient_SetMetrics(g_app.hwnd,
                                      g_app.shell.use_custom_chrome,
                                      g_app.shell.titlebar_height,
                                      g_app.shell.frame_visual_thickness,
                                      g_app.shell.frame_resize_thickness) != 0)
    {
        return 1;
    }
    if (Platform_SetFrameless(g_app.hwnd, g_app.shell.use_custom_chrome) != 0)
        return 1;
    return Platform_RebuildClientArea(g_app.hwnd);
}

/* === Shell-3a_1: 托盘图标生命周期 === */

int App_InitTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    HICON hIcon;

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    wcsncpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), L"DeskNote", _TRUNCATE);

    hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
    if (hIcon == NULL)
        hIcon = LoadIconW(NULL, IDI_APPLICATION);
    nid.hIcon = hIcon;

    App_UpdateTrayTip(hwnd);  /* Shell-5c: 启动时设置初始提示 */
    return Shell_NotifyIconW(NIM_ADD, &nid) ? 0 : 1;
}

/* Shell-5c: 更新托盘图标 hover 提示，反映当前 resident_mode */
void App_UpdateTrayTip(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;

    AppShellResidentMode mode = App_GetResidentMode();
    const wchar_t* tip = L"DeskNote";
    if (mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
        tip = L"DeskNote — 浮动置顶";
    else if (mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
        tip = L"DeskNote — 贴边占位";

    wcsncpy(nid.szTip, tip, 128);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void App_DestroyTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

/* === Shell-3b: presence 状态管理 === */

static ShellPresenceState s_presence_state = SHELL_PRESENCE_VISIBLE_FRONT;

ShellPresenceState App_GetPresenceState(void)
{
    return s_presence_state;
}

AppShellResidentMode App_GetResidentMode(void)
{
    return g_app.shell.resident_mode;
}

void App_SetResidentMode(AppShellResidentMode mode)
{
    g_app.shell.resident_mode = mode;
}

void App_HideToTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_HIDDEN_TO_TRAY;

    StateData state;
    StateStore_Load(&state);

    /* 隐藏时释放 AppBar 死区，恢复时自动重建（repair-5-c） */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED &&
        AppBar_IsRegistered(hwnd))
    {
        AppBar_Unregister(hwnd);
        R5A_WriteLog(R5A_LOG_PREFIX L"App_HideToTray: unregistered AppBar\n");
    }

    ShowWindow(hwnd, SW_HIDE);

    state.presence_state = (int)SHELL_PRESENCE_HIDDEN_TO_TRAY;
    StateStore_Save(&state);
}

/* repair-5-c: 拖动/大小调整结束时释放 AppBar */
void App_OnEndDrag(HWND hwnd)
{
    if (g_app.shell.resident_mode != APP_SHELL_RESIDENT_MODE_EDGE_RESERVED)
        return;
    if (!AppBar_IsRegistered(hwnd))
        return;

    StateData state;
    StateStore_Load(&state);

    if (state.release_on_drag_mode == 1)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        state.last_floating_left   = rc.left;
        state.last_floating_top    = rc.top;
        state.last_floating_width  = rc.right - rc.left;
        state.last_floating_height = rc.bottom - rc.top;
        state.shell_resident_mode  = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
        StateStore_Save(&state);
        AppBar_Unregister(hwnd);
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST;
        SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
        R5A_WriteLog(R5A_LOG_PREFIX L"App_OnEndDrag: released AppBar -> floating_topmost\n");
    }
    else if (state.release_on_drag_mode == 2)
    {
        state.shell_resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
        StateStore_Save(&state);
        AppBar_Unregister(hwnd);
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
        R5A_WriteLog(R5A_LOG_PREFIX L"App_OnEndDrag: released AppBar -> none\n");
    }
    App_UpdateTrayTip(hwnd);  /* Shell-5c */
}

void App_RestoreFromTray(HWND hwnd)
{
    s_presence_state = SHELL_PRESENCE_VISIBLE_FRONT;
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    /* Shell-3c_2: 持久化当前 presence 状态 */
    StateData state;
    StateStore_Load(&state);
    state.presence_state = (int)SHELL_PRESENCE_VISIBLE_FRONT;
    StateStore_Save(&state);

    /* 如果之前是贴边模式但 AppBar 已被释放，把 resident_mode 切回 NONE 保持状态一致 */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_EDGE_RESERVED &&
        !AppBar_IsRegistered(hwnd))
    {
        g_app.shell.resident_mode = APP_SHELL_RESIDENT_MODE_NONE;
        R5A_WriteLog(R5A_LOG_PREFIX L"App_RestoreFromTray: reset resident_mode to NONE (AppBar not registered)\n");
    }

    /* Shell-4a_1: 恢复 topmost（如果之前是置顶模式）—此处只做判断，不直接调系统 API */
    if (g_app.shell.resident_mode == APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    App_UpdateTrayTip(hwnd);  /* Shell-5c */
}
