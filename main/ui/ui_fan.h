/*
 * @file ui_fan.h
 * @brief UI screen for fan control - Public API
 *
 * This header file defines the public interface for the fan control UI screen.
 * It provides functions to initialize and delete the fan control screen.
 * The screen allows users to adjust fan speed using an arc slider.
 */
#ifndef UI_FAN_H__
#define UI_FAN_H__

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the fan control UI screen.
 *
 * This function creates all the LVGL objects for the fan control interface.
 * It sets up the main page, an arc slider to control the fan speed, and
 * labels to display the "Fan" title, percentage symbol, and the current
 * fan speed value. Event callbacks are registered for interaction (e.g.,
 * long press to exit).
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the fan UI is exited
 *               (e.g., by a long press). This function is responsible for
 *               returning to the previous screen, typically the main menu.
 */
void ui_fan_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the fan control UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group and deletes the
 * main page object along with all its children (arc slider, labels).
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_fan_delete(void);

#ifdef __cplusplus
}
#endif

#endif