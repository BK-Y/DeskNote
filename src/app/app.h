#ifndef DESKNOTE_APP_H
#define DESKNOTE_APP_H

#include <windows.h>

/**
 * @defgroup app App Layer
 * Application lifecycle, state management, and command dispatch.
 * Orchestrates all upper-layer modules and provides the public API for
 * platform callbacks.
 * @{
 */

/* ------------------------------------------------------------------ */
/*  Shell Resident Mode                                                */
/* ------------------------------------------------------------------ */

/** @brief Window resident mode. */
typedef enum {
    APP_SHELL_RESIDENT_MODE_NONE = 0,          /**< Normal window. */
    APP_SHELL_RESIDENT_MODE_FLOATING_TOPMOST,  /**< Always-on-top floating window. */
    APP_SHELL_RESIDENT_MODE_EDGE_RESERVED      /**< AppBar edge-docked window. */
} AppShellResidentMode;

/* ------------------------------------------------------------------ */
/*  Shell Command Model                                                */
/* ------------------------------------------------------------------ */

/** @brief Commands that can be submitted to the shell command queue. */
typedef enum {
    APP_SHELL_COMMAND_NONE = 0,                  /**< No command. */
    APP_SHELL_COMMAND_SHOW_MENU,                 /**< Show the titlebar context menu. */
    APP_SHELL_COMMAND_WINDOW_CLOSE,              /**< Close / hide window. */
    APP_SHELL_COMMAND_WINDOW_RESTORE,            /**< Restore from tray. */
    APP_SHELL_COMMAND_TOGGLE_FLOATING_TOPMOST,   /**< Toggle floating topmost mode. */
    APP_SHELL_COMMAND_ENTER_EDGE_RESERVED,        /**< Enter/leave edge-reserved mode. */
    APP_SHELL_COMMAND_HIDE_TO_TRAY,              /**< Hide window to system tray. */
    APP_SHELL_COMMAND_RESTORE_FROM_TRAY,         /**< Restore window from tray. */
    APP_SHELL_COMMAND_SHOW_TRAY_MENU             /**< Show tray icon context menu. */
} AppShellCommand;

/** @brief Titlebar command groups bitmask. */
typedef enum {
    APP_TITLEBAR_COMMAND_GROUP_WINDOW_CONTROLS    = (1u << 0), /**< Window close/restore. */
    APP_TITLEBAR_COMMAND_GROUP_SHELL_MODES        = (1u << 1), /**< Shell mode toggles. */
    APP_TITLEBAR_COMMAND_GROUP_BACKGROUND_ACTIONS = (1u << 2)  /**< Tray actions. */
} AppTitlebarCommandGroup;

/** @brief Presence state (where the window is). */
typedef enum {
    SHELL_PRESENCE_VISIBLE_FRONT,    /**< Window visible and in foreground. */
    SHELL_PRESENCE_HIDDEN_TO_TRAY,   /**< Window hidden to system tray. */
    SHELL_PRESENCE_EXITING           /**< Application is shutting down. */
} ShellPresenceState;

/* ------------------------------------------------------------------ */
/*  Shell State                                                        */
/* ------------------------------------------------------------------ */

/** @brief Runtime shell state (not persisted). */
typedef struct {
    int use_custom_chrome;                /**< Whether frameless chrome is active. */
    int titlebar_height;                  /**< Titlebar height in pixels. */
    int frame_visual_thickness;           /**< Visual border thickness (px). */
    int frame_resize_thickness;           /**< Resize hit-test border (px). */
    AppShellResidentMode resident_mode;   /**< Current resident mode. */
    unsigned int titlebar_command_groups; /**< Bitmask of active command groups. */
    AppShellCommand active_command;       /**< Currently queued shell command. */
} AppShellState;

/* ------------------------------------------------------------------ */
/*  Save State                                                         */
/* ------------------------------------------------------------------ */

/** @brief Auto-save snapshot. */
typedef struct {
    unsigned int working_version;    /**< Current in-memory version. */
    unsigned int persisted_version;  /**< Last persisted version. */
    unsigned int recovery_version;   /**< Last recovery snapshot version. */
    int is_dirty;                    /**< Whether unsaved changes exist. */
} AppSaveState;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/** @brief Check if the document has unsaved changes.
 *  @return 1 if dirty, 0 otherwise. */
int App_IsDirty(void);

/** @brief Get current save state snapshot. */
int App_GetSaveState(AppSaveState* out_state);

/** @brief Get current shell state. */
int App_GetShellState(AppShellState* out_state);

/** @brief Submit a shell command for async execution. */
int App_SubmitShellCommand(AppShellCommand command);

/** @brief Retrieve and clear the pending shell command (called in timer). */
int App_TakePendingShellCommand(AppShellCommand* out_command);

/** @brief Get current resident mode. */
AppShellResidentMode App_GetResidentMode(void);

/** @brief Set current resident mode (runtime only, no persistence). */
void App_SetResidentMode(AppShellResidentMode mode);

/** @brief Get current presence state. */
ShellPresenceState App_GetPresenceState(void);

/** @brief Hide window to system tray. */
void App_HideToTray(HWND hwnd);

/** @brief Restore window from system tray. */
void App_RestoreFromTray(HWND hwnd);

/** @brief Handle drag-end: release AppBar if in edge-reserved mode. */
void App_OnEndDrag(HWND hwnd);

/** @brief Update tray tooltip text to reflect current resident mode. */
void App_UpdateTrayTip(HWND hwnd);

/* ------------------------------------------------------------------ */
/*  Application Lifecycle                                               */
/* ------------------------------------------------------------------ */

/** @brief Create the window and run the message loop.
 *  @return wParam from the final WM_QUIT message. */
int App_Run(void);

/** @brief Initialize the application.
 *  @param hwnd The main window handle (must be non-NULL).
 *  @return 0 on success, 1 on failure. */
int App_Init(HWND hwnd);

/** @brief Shut down the application and release resources. */
void App_Shutdown(void);

/* ------------------------------------------------------------------ */
/*  Window Event Handlers (called from platform layer)                  */
/* ------------------------------------------------------------------ */

/** @brief Handle window resize. */
int App_OnResize(unsigned int width, unsigned int height);

/** @brief Handle WM_PAINT: render the full frame. */
void App_OnPaint(void);

/** @brief Handle character input. */
void App_OnChar(wchar_t ch);

/** @brief Handle key down (non-character keys). */
void App_OnKeyDown(unsigned int key);

/** @brief Handle left mouse button down for hit testing. */
void App_OnLeftButtonDown(int x, int y);

/** @brief Handle mouse move (for hover states). */
void App_OnMouseMove(int x, int y);

/** @brief Handle mouse leave (reset hover states). */
void App_OnMouseLeave(void);

/** @brief Handle mouse wheel for scrolling. */
void App_OnMouseWheel(int delta);

/** @brief Called on each auto-save timer tick. */
void App_OnTick(void);

/** @brief Handle window focus gained. */
void App_OnFocusGained(void);

/** @brief Handle window focus lost. */
void App_OnFocusLost(void);

/** @brief Handle IME composition start (position caret). */
void App_OnImeStartComposition(void);

/* ------------------------------------------------------------------ */
/*  Tray Icon                                                           */
/* ------------------------------------------------------------------ */

/** @brief Create the system tray icon. */
int App_InitTrayIcon(HWND hwnd);

/** @brief Destroy the system tray icon. */
void App_DestroyTrayIcon(HWND hwnd);

/* ------------------------------------------------------------------ */
/*  Shell Utilities                                                     */
/* ------------------------------------------------------------------ */

/** @brief Get the command group for a shell command. */
int App_GetShellCommandGroup(AppShellCommand command, AppTitlebarCommandGroup* out_group);

/** @brief Enable or disable custom (frameless) chrome.
 *  @note Currently always enabled in production.
 *  @return 0 on success. */
int App_EnableCustomChrome(int enable);

/** @brief Request a full client area rebuild (re-layout).
 *  @return 0 on success. */
int App_RequestClientRebuild(void);

/** @} */
#endif /* DESKNOTE_APP_H */
