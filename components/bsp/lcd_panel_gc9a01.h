/*
 * @file lcd_panel_gc9a01.h
 * @brief GC9A01 LCD panel driver - Public API
 *
 * This header file defines the public interface for creating and configuring
 * a GC9A01 LCD panel driver instance.
 *
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new GC9A01 LCD panel.
 *
 * This function allocates and initializes a new GC9A01 panel structure.
 * It configures GPIO for reset, sets up color space and pixel format,
 * and registers panel operation functions. This is part of the public API,
 * intended to be called by the BSP or application to initialize the LCD driver.
 *
 * @param io Handle to the LCD panel IO interface.
 * @param panel_dev_config Pointer to the panel device configuration structure.
 * @param ret_panel Pointer to receive the handle of the new panel.
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: If any argument is invalid (e.g., io, panel_dev_config, or ret_panel is NULL).
 *     - ESP_ERR_NO_MEM: If memory allocation fails for the gc9a01_panel_t structure.
 *     - ESP_ERR_NOT_SUPPORTED: If the specified color_space or bits_per_pixel in panel_dev_config is not supported by this driver.
 *     - Other ESP_ERR codes from lower-level functions like gpio_config.
 */
esp_err_t lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif
