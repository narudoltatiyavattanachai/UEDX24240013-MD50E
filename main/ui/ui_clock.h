/*
 * @file ui_clock.h
 * @brief UI screen for analog clock - Public API
 *
 * This header file defines the public interface for the analog clock UI screen.
 * It provides functions to initialize and delete the clock screen.
 * The clock screen displays an analog clock face with hour, minute, and
 * second hands, updating in real-time.
 */
#ifndef UI_CLOCK_H
#define UI_CLOCK_H

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the clock UI screen.
 *
 * This function creates all the LVGL objects for the analog clock display.
 * It sets up the main page, the meter widget for the clock face, scales for
 * hours and minutes, and needle indicators for hours, minutes, and seconds.
 * An LVGL timer is created to update the clock periodically.
 * Event callbacks are registered for interaction (e.g., long press to exit).
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the clock UI is exited
 *               (e.g., by a long press). This function is responsible for
 *               returning to the previous screen, typically the main menu.
 */
void ui_clock_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the clock UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group, deletes the
 * LVGL timer used for clock updates, and deletes the main page object
 * along with its children (the clock meter and its components).
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_clock_delete(void);

#ifdef __cplusplus
}
#endif

#endif