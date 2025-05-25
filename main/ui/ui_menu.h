/*
 * @file ui_menu.h
 * @brief Main menu UI - Public API
 *
 * This header file defines the public interface for the main menu UI.
 * It provides functions to initialize and, if necessary, delete/deinitialize
 * the main menu screen. This is typically the entry point for the user to
 * access different applications or features of the device.
 */
#ifndef UI_MENU_H__
#define UI_MENU_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the main menu UI.
 *
 * This function creates the main menu page, sets up the background image,
 * and initializes the display of application icons in a rotating list.
 * It positions the initial set of icons, applies zoom effects, and sets up
 * the invisible icon used for smooth animation transitions. Event callbacks
 * for menu navigation (focus, key press, click) are registered to the
 * background image object. The background image is then added to the
 * default encoder group to make it focusable.
 * This is part of the public API, typically called from `ui_init`.
 */
void ui_menu_init(void);

/**
 * @brief Deletes or deinitializes the main menu UI.
 *
 * (Note: The current implementation of this function is empty.
 * If specific cleanup is required for the menu, such as freeing allocated
 * resources or unregistering event callbacks, it should be added here.)
 * This function is intended to be part of the public API for managing the
 * lifecycle of the menu UI.
 */
void ui_menu_delete(void);

#ifdef __cplusplus
}
#endif

#endif