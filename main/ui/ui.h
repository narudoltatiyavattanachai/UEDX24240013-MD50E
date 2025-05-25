/*
 * @file ui.h
 * @brief Core UI management - Public API
 *
 * This header file defines the public interface for the core UI management,
 * primarily related to LVGL input group handling for encoder navigation.
 * It provides functions to initialize the UI, and to add or remove LVGL
 * objects from the main encoder group, which is essential for managing
 * focus across different UI components.
 */
#ifndef UI_UI_H__
#define UI_UI_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic callback function type.
 *
 * This type defines a generic callback function that takes a void pointer
 * as an argument and returns nothing. It can be used for various event
 * handling or callback mechanisms within the UI.
 *
 * @param args A void pointer to user-defined arguments passed to the callback.
 */
typedef void (*ret_cb_t)(void *args);

/**
 * @brief Initializes the core UI components.
 *
 * This function creates a default LVGL group and associates it with the
 * first available input device if it's an encoder. This group is then
 * used for managing focus for navigable UI elements. It also calls
 * `ui_menu_init()` to initialize the main menu UI.
 * This is part of the public API.
 */
void ui_init(void);

/**
 * @brief Adds an LVGL object to the main encoder input group.
 *
 * This function allows UI elements to be made focusable and navigable
 * via the encoder. This is part of the public API.
 *
 * @param obj Pointer to the LVGL object to be added to the group.
 *            The object must not be NULL.
 */
void ui_add_obj_to_encoder_group(lv_obj_t *obj);

/**
 * @brief Removes all objects from the main encoder input group.
 *
 * This function is typically used when changing UI screens or contexts,
 * to clear the previous set of focusable objects from the main encoder group.
 * This is part of the public API.
 */
void ui_remove_all_objs_from_encoder_group(void);

#ifdef __cplusplus
}
#endif

#endif
