#ifndef DESKNOTE_APPBAR_H
#define DESKNOTE_APPBAR_H

#include <windows.h>

/**
 * @defgroup platform_win32 Win32 Platform
 * Win32 window management, message loop, non-client handling, and AppBar.
 * @{
 */

/**
 * @defgroup appbar AppBar
 * Windows AppBar (application desktop toolbar) registration and positioning.
 * Each function is safe to call even when the AppBar is not registered
 * (they check the internal registration state first).
 * @{
 */

/** @brief AppBar docking edge (matches Windows ABE_* constants). */
typedef enum {
    APP_DOCK_LEFT   = 0,  /**< Dock to left edge   (ABE_LEFT). */
    APP_DOCK_TOP    = 1,  /**< Dock to top edge    (ABE_TOP). */
    APP_DOCK_RIGHT  = 2,  /**< Dock to right edge  (ABE_RIGHT). */
    APP_DOCK_BOTTOM = 3   /**< Dock to bottom edge (ABE_BOTTOM). */
} AppDockEdge;

/** @brief Register the window as an AppBar.
 *  @param hwnd Window handle.
 *  @return 0 on success, 1 if SHAppBarMessage(ABM_NEW) fails. */
int AppBar_Register(HWND hwnd);

/** @brief Unregister the AppBar and release the reserved screen area.
 *  @param hwnd Window handle.
 *  @return 0 always. */
int AppBar_Unregister(HWND hwnd);

/** @brief Set the AppBar position and reserve screen space.
 *  @param hwnd    Window handle.
 *  @param edge    Edge to dock to.
 *  @param thickness  Reservation thickness in pixels.
 *  @return 0 always. */
int AppBar_SetPosition(HWND hwnd, AppDockEdge edge, int thickness);

/** @brief Check whether the AppBar is currently registered.
 *  @return 1 if registered, 0 otherwise. */
int AppBar_IsRegistered(HWND hwnd);

/** @brief Re-register the AppBar (unregister then register + set position).
 *  @return 0 on success, 1 if re-registration fails. */
int AppBar_ReRegister(HWND hwnd);

/** @brief Read dock configuration from the persisted state file.
 *  @param[out] out_edge      Dock edge.
 *  @param[out] out_thickness  Dock thickness.
 *  @return 0 always. */
int AppBar_ReadDockConfig(AppDockEdge* out_edge, int* out_thickness);

/** @brief Get the current AppBar docking edge.
 *  @return Current edge (APP_DOCK_RIGHT if not registered). */
AppDockEdge AppBar_GetEdge(void);

/** @brief Get the current AppBar reservation thickness.
 *  @return Thickness in pixels (0 if not registered). */
int AppBar_GetThickness(void);

/** @brief Mark the next work-area change as self-triggered.
 *  Used by AppBar_SetPosition before ABM_SETPOS to prevent
 *  feedback loops in WM_SETTINGCHANGE. */
void AppBar_MarkOwnWorkareaChange(void);

/** @brief Check and consume the self-triggered work-area change flag.
 *  @return 1 if the current WM_SETTINGCHANGE should be skipped. */
int AppBar_ConsumeOwnWorkareaChange(void);

/** @} */
/** @} */
#endif /* DESKNOTE_APPBAR_H */
