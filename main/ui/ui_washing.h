/*
 * @file ui_washing.h
 * @brief UI screen for a washing machine - Public API
 *
 * This header file defines the public interface for the washing machine UI screen.
 * It provides functions to initialize and delete the washing machine screen.
 * The screen displays selectable wash cycles with animations.
 */
#ifndef UI_WASHING_H__
#define UI_WASHING_H__

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the washing machine UI screen.
 *
 * This function creates all the LVGL objects for the washing machine interface.
 * It sets up the main page, background, animated waves and bubbles, and a
 * circular arrangement of selectable wash function icons. Event callbacks
 * are registered for interaction (e.g., long press to exit, key press to
 * select wash cycle).
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the washing UI is exited
 *               (e.g., by a long press). This function is responsible for
 *               returning to the previous screen, typically the main menu.
 */
void ui_washing_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the washing machine UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group, stops any running
 * animations related to this UI, and deletes the main page object along with
 * all its children.
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_washing_delete(void);

#ifdef __cplusplus
}
#endif

#endif