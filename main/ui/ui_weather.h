/*
 * @file ui_weather.h
 * @brief UI screen for weather display - Public API
 *
 * This header file defines the public interface for the weather display UI screen.
 * It provides functions to initialize and delete the weather screen.
 * The screen displays weather information such as city, temperature, and conditions.
 */
#ifndef UI_WEATHER_H__
#define UI_WEATHER_H__

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the weather display UI screen.
 *
 * This function creates all the LVGL objects for the weather display.
 * It sets up the main page, background image, and labels for city name,
 * temperature, weather icon, and weather state (including min/max temps).
 * Entry animations are applied to elements. The page is made focusable
 * for encoder interaction.
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the weather UI is exited
 *               (e.g., by a long press). This function is responsible for
 *               returning to the previous screen, typically the main menu.
 */
void ui_weather_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the weather display UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group, stops any running
 * animations related to this UI, and deletes the main page object along with
 * all its children.
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_weather_delete(void);

#ifdef __cplusplus
}
#endif

#endif