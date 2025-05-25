/*
 * @file bsp_lcd.h
 * @brief Board Support Package for LCD control - Public API
 *
 * This header file defines the public interface for interacting with the LCD
 * display. It includes definitions for LCD resolution, callback types, and
 * function prototypes for initializing and controlling the LCD.
 */
#ifndef BSP_LCD_H
#define BSP_LCD_H

#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Horizontal resolution of the LCD
#define LCD_H_RES               (240)
// Vertical resolution of the LCD
#define LCD_V_RES               (240)

/**
 * @brief Callback function type for LCD transaction done event.
 *
 * @return true if a high-priority task has been waken up by this callback, false otherwise.
 *         This return value is important for the underlying FreeRTOS interrupt handling.
 */
typedef bool (*bsp_lcd_trans_done_cb_t)(void);

/**
 * @brief Initializes the LCD panel.
 *
 * This function initializes the SPI bus, installs the LCD panel IO and driver (GC9A01),
 * configures the backlight PWM, and sets up the TE (Tearing Effect) line if configured.
 * This is part of the public API.
 *
 * @return esp_lcd_panel_handle_t Handle to the initialized LCD panel, or NULL on failure.
 */
esp_lcd_panel_handle_t bsp_lcd_init(void);

/**
 * @brief Registers a callback function to be called when a color data transaction is done.
 *
 * This is part of the public API.
 *
 * @param callback The callback function to register.
 */
void  bsp_lcd_trans_done_cb_register(bsp_lcd_trans_done_cb_t callback);

/**
 * @brief Sets the brightness of the LCD backlight.
 *
 * This function controls the LCD backlight brightness using LEDC PWM.
 * This is part of the public API.
 *
 * @param percent Brightness percentage (0-100). Values outside this range will be clamped.
 */
void bsp_lcd_set_brightness(uint8_t percent);

/**
 * @brief Waits for the LCD flush ready signal.
 *
 * This function is used when the TE (Tearing Effect) line is enabled. It blocks
 * until the TE line signals that the LCD is ready for a new frame.
 * This is part of the public API.
 */
void bsp_lcd_wait_flush_ready(void);

#ifdef __cplusplus
}
#endif

#endif