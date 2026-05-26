#include "app.h"
#include "../editor/editor.h"
#include "../platform/win32/window.h"
#include "../render/render.h"
#include "../storage/note_store.h"
#include "../storage/state_store.h"
#include "../ui/editor_view.h"
#include <imm.h>
#include <string.h>
#include <wchar.h>

typedef struct {
    HWND hwnd;
    RenderContext* render;
    Editor editor;
    int editor_ready;
    int has_focus;
    wchar_t current_file_path[MAX_PATH];
} AppState;

static AppState g_app;

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

static void App_ReportStorageFailure(const wchar_t* message)
{
    if (message == NULL)
        return;

    OutputDebugStringW(message);
}

static int App_LoadInitialDocument(void)
{
    StateData state;
    wchar_t default_path[MAX_PATH];
    wchar_t* loaded_text;
    const wchar_t* target_path;

    if (NoteStore_GetDefaultNotePath(default_path, MAX_PATH) != 0)
        return 1;

    memset(&state, 0, sizeof(state));
    loaded_text = NULL;
    target_path = default_path;

    if (StateStore_Load(&state) != 0)
        App_ReportStorageFailure(L"DeskNote: failed to load state.ini, falling back to default note path.\n");

    if (state.has_last_file && state.last_file[0] != L'\0')
        target_path = state.last_file;

    if (App_SetCurrentFilePath(target_path) != 0)
        return 1;

    if (NoteStore_LoadText(g_app.current_file_path, &loaded_text) != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to load note file, starting with empty document.\n");
        return Editor_SetText(&g_app.editor, L"");
    }

    if (Editor_SetText(&g_app.editor, loaded_text) != 0)
    {
        NoteStore_FreeText(loaded_text);
        return 1;
    }

    NoteStore_FreeText(loaded_text);
    return 0;
}

static int App_SaveCurrentDocument(void)
{
    StateData state;

    if (!g_app.editor_ready || g_app.current_file_path[0] == L'\0')
        return 1;

    if (NoteStore_SaveText(g_app.current_file_path,
                           Document_GetText(Editor_GetDocument(&g_app.editor))) != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to save note file.\n");
        return 1;
    }

    memset(&state, 0, sizeof(state));
    state.version = 1;
    state.has_last_file = 1;
    if (wcslen(g_app.current_file_path) >= MAX_PATH)
        return 1;
    memcpy(state.last_file,
           g_app.current_file_path,
           (wcslen(g_app.current_file_path) + 1) * sizeof(wchar_t));

    if (StateStore_Save(&state) != 0)
    {
        App_ReportStorageFailure(L"DeskNote: failed to save state.ini.\n");
        return 1;
    }

    return 0;
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
    unsigned int width;
    unsigned int height;

    if (!g_app.editor_ready || !g_app.has_focus || g_app.hwnd == NULL || g_app.render == NULL)
        return;
    if (App_GetClientSize(&width, &height) != 0)
        return;
    if (EditorView_GetCaretRect(g_app.render,
                                Editor_GetDocument(&g_app.editor),
                                Editor_GetCursor(&g_app.editor),
                                (int)width,
                                (int)height,
                                &caret_rect) != 0)
        return;

    DestroyCaret();
    if (!CreateCaret(g_app.hwnd, NULL, caret_rect.width, caret_rect.height))
        return;

    SetCaretPos(caret_rect.x, caret_rect.y);
    ShowCaret(g_app.hwnd);
    App_UpdateImeWindow(caret_rect);
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
    g_app.current_file_path[0] = L'\0';

    if (App_LoadInitialDocument() != 0)
    {
        Editor_Free(&g_app.editor);
        g_app.editor_ready = 0;
        Render_Destroy(g_app.render);
        g_app.render = NULL;
        return 1;
    }

    return 0;
}

void App_Shutdown(void)
{
    if (g_app.editor_ready)
    {
        (void)App_SaveCurrentDocument();
        Editor_Free(&g_app.editor);
        g_app.editor_ready = 0;
    }

    DestroyCaret();
    g_app.has_focus = 0;

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

    App_UpdateInputCaret();
    return 0;
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
    EditorView_Draw(g_app.render,
                    Editor_GetDocument(&g_app.editor),
                    Editor_GetCursor(&g_app.editor),
                    (int)width,
                    (int)height);
    (void)Render_EndFrame(g_app.render);
    App_UpdateInputCaret();
}

void App_OnChar(wchar_t ch)
{
    int changed = 1;

    if (!g_app.editor_ready || g_app.hwnd == NULL)
        return;

    if (ch == L'\r')
        changed = Editor_HandleKey(&g_app.editor, EDITOR_KEY_ENTER);
    else if (ch >= L' ')
        changed = Editor_HandleChar(&g_app.editor, ch);

    if (changed == 0)
    {
        App_UpdateInputCaret();
        InvalidateRect(g_app.hwnd, NULL, FALSE);
    }
}

void App_OnKeyDown(unsigned int key)
{
    int changed = 1;

    if (!g_app.editor_ready || g_app.hwnd == NULL)
        return;

    switch (key)
    {
    case VK_LEFT:
        changed = Editor_HandleKey(&g_app.editor, EDITOR_KEY_LEFT);
        break;

    case VK_RIGHT:
        changed = Editor_HandleKey(&g_app.editor, EDITOR_KEY_RIGHT);
        break;

    case VK_BACK:
        changed = Editor_HandleKey(&g_app.editor, EDITOR_KEY_BACKSPACE);
        break;
    }

    if (changed == 0)
    {
        App_UpdateInputCaret();
        InvalidateRect(g_app.hwnd, NULL, FALSE);
    }
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
