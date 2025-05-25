/*
 * @file ui_player.h
 * @brief UI screen for a music player - Public API
 *
 * This header file defines the public interface for the music player UI screen.
 * It provides functions to initialize and delete the player screen.
 * The screen displays track information, playback controls (play/pause, next,
 * previous), and volume/progress indicators.
 */
#ifndef UI_PLAYER_H__
#define UI_PLAYER_H__

#include "ui.h" // For ret_cb_t definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the music player UI screen.
 *
 * This function creates all the LVGL objects for the music player interface.
 * It sets up the main page, displays track information (artist, album),
 * playback progress, volume control, and control buttons (play/pause,
 * next, previous, mode). Entry animations are applied to elements.
 * Interactive elements are added to the encoder group for navigation.
 * This is part of the public API for this UI component.
 *
 * @param ret_cb Callback function to be executed when the player UI is exited
 *               (e.g., by a long press on a control button). This function
 *               is responsible for returning to the previous screen,
 *               typically the main menu.
 */
void ui_player_init(ret_cb_t ret_cb);

/**
 * @brief Deletes the music player UI screen and frees associated resources.
 *
 * This function removes all objects from the encoder group, stops any running
 * animations related to this UI, and deletes the main page object along with
 * all its children (arcs, labels, buttons, etc.).
 * After cleanup, it calls the `return_callback` function that was provided
 * during initialization, allowing a return to the previous UI screen.
 * This is part of the public API for this UI component.
 */
void ui_player_delete(void);

#ifdef __cplusplus
}
#endif

#endif
