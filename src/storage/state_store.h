#ifndef DESKNOTE_STATE_STORE_H
#define DESKNOTE_STATE_STORE_H

#include <windows.h>

/**
 * @defgroup storage Storage
 * Persistent state and note file management in %LOCALAPPDATA%\\DeskNote\\.
 * @{
 */

/** @brief Persisted application state, read/written as state.ini (UTF-16 LE). */
typedef struct {
    int version;                     /**< Format version (currently 2). */
    int has_last_file;               /**< Whether last_file is valid. */
    wchar_t last_file[MAX_PATH];     /**< Path of the last opened note. */
    int use_custom_chrome;           /**< Frameless chrome flag (fixed at 1). */
    int titlebar_height;             /**< Titlebar height in pixels. */
    int frame_visual_thickness;      /**< Border visual thickness (px). */
    int shell_resident_mode;         /**< @ref AppShellResidentMode value. */
    int presence_state;              /**< @ref ShellPresenceState value. */
    int last_floating_left;          /**< Last floating window X position. */
    int last_floating_top;           /**< Last floating window Y position. */
    int last_floating_width;         /**< Last floating window width. */
    int last_floating_height;        /**< Last floating window height. */
    int dock_edge;                   /**< AppBar dock edge @ref AppDockEdge. */
    int dock_thickness;              /**< AppBar reservation thickness (px). */
    int release_on_hide_mode;        /**< Hide-release policy (0=never,1=remember,2=clear). */
    int release_on_drag_mode;        /**< Drag-release policy (0=never,1=to_topmost,2=to_none). */
} StateData;

/** @brief Get the root data directory path (e.g. %LOCALAPPDATA%\\DeskNote).
 *  @param[out] buffer       Output buffer.
 *  @param      buffer_count Buffer size in wchar_t units.
 *  @return 0 on success. */
int StateStore_GetRootPath(wchar_t* buffer, int buffer_count);

/** @brief Get the full state.ini file path.
 *  @param[out] buffer       Output buffer.
 *  @param      buffer_count Buffer size in wchar_t units.
 *  @return 0 on success. */
int StateStore_GetStatePath(wchar_t* buffer, int buffer_count);

/** @brief Load state from state.ini.
 *  Missing fields are initialized to defaults; missing file returns 0.
 *  @param[out] out_state Populated StateData.
 *  @return 0 on success. */
int StateStore_Load(StateData* out_state);

/** @brief Save state to state.ini (atomic write).
 *  @param state State to persist.
 *  @return 0 on success. */
int StateStore_Save(const StateData* state);

/** @} */
#endif /* DESKNOTE_STATE_STORE_H */
