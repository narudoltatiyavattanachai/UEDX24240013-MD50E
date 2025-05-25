/*
 * @file ui_light.h
 * @brief UI screen for light control - Public API
 *
 * This header file defines the public interface for the light control UI screen.
 * It provides functions to initialize, update, and delete the light control screen.
 * The screen allows users to adjust light brightness and color via a tabbed interface.
 */
#ifndef UI_LIGHT_H
#define UI_LIGHT_H

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the light control UI screen.
 *
 * This function creates all the LVGL objects for the light control interface.
 * It sets up a tab view with three tabs:
 * - Tab 1: Contains an arc slider for brightness control.
 * - Tab 2: Contains a color wheel for selecting light color.
 * - Tab 3: Placeholder tab.
 * Event callbacks are registered for interaction (e.g., long press to exit
 * from interactive elements).
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the light UI is exited
 *               (e.g., by a long press). This function is responsible for
 *               returning to the previous screen, typically the main menu.
 */
void ui_light_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the light control UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group and deletes the
 * main page object along with all its children (tab view, tabs, arc slider,
 * color wheel, etc.).
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_light_delete(void);

/**
 * @brief Sets the brightness value on the light UI.
 *
 * This function allows external modules to programmatically set the brightness
 * value displayed and controlled by the arc slider on the light UI's first tab.
 * This is part of the public API for this UI component.
 *
 * @param value The brightness value to set (typically 0-100).
 */
void ui_light_set_brightness(uint8_t value);


#ifdef __cplusplus
}
#endif

#endif