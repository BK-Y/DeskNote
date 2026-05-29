#ifndef APP_H
#define APP_H

#include <windows.h>

typedef struct {
    unsigned int working_version;
    unsigned int persisted_version;
    unsigned int recovery_version;
    int is_dirty;
} AppSaveState;

typedef enum {
    APP_TITLEBAR_COMMAND_GROUP_NONE = 0,
    APP_TITLEBAR_COMMAND_GROUP_WINDOW_CONTROLS = 1u << 0,
    APP_TITLEBAR_COMMAND_GROUP_SHELL_MODES = 1u << 1,
    APP_TITLEBAR_COMMAND_GROUP_BACKGROUND_ACTIONS = 1u << 2
} AppTitlebarCommandGroup;

typedef enum {
    APP_SHELL_RESIDENT_MODE_NONE = 0
} AppShellResidentMode;

typedef enum {
    APP_SHELL_COMMAND_NONE = 0,
    APP_SHELL_COMMAND_SHOW_MENU,
    APP_SHELL_COMMAND_WINDOW_CLOSE,
    APP_SHELL_COMMAND_WINDOW_RESTORE,
    APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST,
    APP_SHELL_COMMAND_ENTER_EDGE_RESERVED,
    APP_SHELL_COMMAND_HIDE_TO_TRAY,
    APP_SHELL_COMMAND_RESTORE_FROM_TRAY,
    APP_SHELL_COMMAND_SHOW_TRAY_MENU
} AppShellCommand;

/* Shell-3b: 壳层 presence 状态 */
typedef enum {
    SHELL_PRESENCE_VISIBLE_FRONT,
    SHELL_PRESENCE_HIDDEN_TO_TRAY,
    SHELL_PRESENCE_EXITING
} ShellPresenceState;

typedef struct {
    int use_custom_chrome;
    int titlebar_height;
    int frame_visual_thickness;
    int frame_resize_thickness;
    AppShellResidentMode resident_mode;
    unsigned int titlebar_command_groups;
    AppShellCommand active_command;
} AppShellState;

int App_Run(void);
int App_Init(HWND hwnd);
void App_Shutdown(void);
int App_OnResize(unsigned int width, unsigned int height);
void App_OnPaint(void);
void App_OnChar(wchar_t ch);
void App_OnKeyDown(unsigned int key);
void App_OnLeftButtonDown(int x, int y);
void App_OnMouseMove(int x, int y);
void App_OnMouseLeave(void);
void App_OnMouseWheel(int delta);
void App_OnTick(void);
void App_OnFocusGained(void);
void App_OnFocusLost(void);
void App_OnImeStartComposition(void);
int App_GetSaveState(AppSaveState* out_state);
int App_IsDirty(void);
int App_GetShellState(AppShellState* out_state);
int App_SubmitShellCommand(AppShellCommand command);
int App_TakePendingShellCommand(AppShellCommand* out_command);
int App_GetShellCommandGroup(AppShellCommand command, AppTitlebarCommandGroup* out_group);
int App_EnableCustomChrome(int enable);
int App_RequestClientRebuild(void);

/* Shell-3a_1: 托盘图标生命周期 */
int App_InitTrayIcon(HWND hwnd);
void App_DestroyTrayIcon(HWND hwnd);

/* Shell-3b: presence 状态管理 */
ShellPresenceState App_GetPresenceState(void);
void App_HideToTray(HWND hwnd);
void App_RestoreFromTray(HWND hwnd);

#endif
