/*
 * @file lcd_panel_gc9a01.c
 * @brief GC9A01 LCD panel driver implementation
 *
 * This file provides the driver for the GC9A01 LCD panel, including
 * initialization, drawing, and control functions. It implements the
 * esp_lcd_panel_interface.h API for ESP-IDF.
 *
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "lcd_panel.gc9a01";

// Forward declarations for static (internal) panel operation functions
static esp_err_t panel_gc9a01_del(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_init(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_gc9a01_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_gc9a01_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_gc9a01_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_gc9a01_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static esp_err_t panel_gc9a01_disp_on_off(esp_lcd_panel_t *panel, bool on_off);
#else
static esp_err_t panel_gc9a01_disp_off(esp_lcd_panel_t *panel, bool off);
#endif

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    unsigned int bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save current value of LCD_CMD_COLMOD register
} gc9a01_panel_t;

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
 *     - ESP_ERR_INVALID_ARG: If any argument is invalid.
 *     - ESP_ERR_NO_MEM: If memory allocation fails.
 *     - ESP_ERR_NOT_SUPPORTED: If the color space or pixel width is not supported.
 *     - Other ESP_ERR codes from lower-level functions.
 */
esp_err_t lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    gc9a01_panel_t *gc9a01 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gc9a01 = calloc(1, sizeof(gc9a01_panel_t));
    ESP_GOTO_ON_FALSE(gc9a01, ESP_ERR_NO_MEM, err, TAG, "no mem for gc9a01 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->color_space) {
    case ESP_LCD_COLOR_SPACE_RGB:
        gc9a01->madctl_val = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        gc9a01->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16:
        gc9a01->colmod_cal = 0x55;
        break;
    case 18:
        gc9a01->colmod_cal = 0x66;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    gc9a01->io = io;
    gc9a01->bits_per_pixel = panel_dev_config->bits_per_pixel;
    gc9a01->reset_gpio_num = panel_dev_config->reset_gpio_num;
    gc9a01->reset_level = panel_dev_config->flags.reset_active_high;
    gc9a01->base.del = panel_gc9a01_del;
    gc9a01->base.reset = panel_gc9a01_reset;
    gc9a01->base.init = panel_gc9a01_init;
    gc9a01->base.draw_bitmap = panel_gc9a01_draw_bitmap;
    gc9a01->base.invert_color = panel_gc9a01_invert_color;
    gc9a01->base.set_gap = panel_gc9a01_set_gap;
    gc9a01->base.mirror = panel_gc9a01_mirror;
    gc9a01->base.swap_xy = panel_gc9a01_swap_xy;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    gc9a01->base.disp_on_off = panel_gc9a01_disp_on_off;
#else
    gc9a01->base.disp_off = panel_gc9a01_disp_off;
#endif
    *ret_panel = &(gc9a01->base);
    ESP_LOGD(TAG, "new gc9a01 panel @%p", gc9a01);

    return ESP_OK;

err:
    if (gc9a01) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(gc9a01);
    }
    return ret;
}

/**
 * @brief Delete a GC9A01 LCD panel.
 *
 * This function frees the resources associated with a GC9A01 panel,
 * including resetting the reset GPIO pin if used. This function is
 * registered as part of the esp_lcd_panel_t structure and called when
 * esp_lcd_panel_del() is invoked.
 *
 * @param panel Handle to the panel to be deleted.
 * @return
 *     - ESP_OK: Success
 */
static esp_err_t panel_gc9a01_del(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);

    if (gc9a01->reset_gpio_num >= 0) {
        gpio_reset_pin(gc9a01->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del gc9a01 panel @%p", gc9a01);
    free(gc9a01);
    return ESP_OK;
}

/**
 * @brief Reset the GC9A01 LCD panel.
 *
 * This function performs a hardware reset if a reset GPIO is configured,
 * otherwise it performs a software reset using the LCD_CMD_SWRESET command.
 * This function is registered as part of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel to be reset.
 * @return
 *     - ESP_OK: Success
 */
static esp_err_t panel_gc9a01_reset(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;

    // perform hardware reset
    if (gc9a01->reset_gpio_num >= 0) {
        gpio_set_level(gc9a01->reset_gpio_num, gc9a01->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(gc9a01->reset_gpio_num, !gc9a01->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5m before sending new command
    }

    return ESP_OK;
}

/**
 * @brief Structure to define an LCD initialization command.
 */
typedef struct {
    uint8_t cmd;           /*!< LCD command */
    uint8_t data[16];      /*!< Command data */
    uint8_t data_bytes;    /*!< Number of data bytes; 0xFF terminates the command list */
} lcd_init_cmd_t;

// Array of initialization commands for the GC9A01 LCD panel
static const lcd_init_cmd_t vendor_specific_init[] = {
    // Enable Inter Register
    {0xfe, {0x00}, 0},
    {0xef, {0x00}, 0},
    {0xeb, {0x14}, 1},
    {0x84, {0x60}, 1},
    {0x85, {0xff}, 1},
    {0x86, {0xff}, 1},
    {0x87, {0xff}, 1},
    {0x8e, {0xff}, 1},
    {0x8f, {0xff}, 1},
    {0x88, {0x0a}, 1},
    {0x89, {0x21}, 1},
    {0x8a, {0x00}, 1},
    {0x8b, {0x80}, 1},
    {0x8c, {0x01}, 1},
    {0x8d, {0x03}, 1},
    {0xB5, {0x08, 0x09, 0x14, 0x08}, 4},
    {0xB6, {0x00, 0x00}, 2},
    {0x36, {0x48}, 1}, // MADCTL: MX, BGR
    {0x3a, {0x05}, 1}, // COLMOD: 16-bit/pixel (RGB565)
    {0x90, {0x08, 0x08, 0x08, 0x08}, 4},
    {0xbd, {0x06}, 1},
    {0xba, {0x01}, 1},
    {0xbc, {0x00}, 1},
    {0xff, {0x60, 0x01, 0x04}, 3},
    {0xc3, {0x13}, 1},
    {0xc4, {0x13}, 1},
    {0xc9, {0x25}, 1},
    {0xbe, {0x11}, 1},
    {0xe1, {0x10, 0x0e}, 2},
    {0xdf, {0x21, 0x0c, 0x02}, 3},
    {0xf0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2a}, 6}, // Gamma Curve Related
    {0xf1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6f}, 6}, // Gamma Curve Related
    {0xf2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2a}, 6}, // Gamma Curve Related
    {0xf3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6f}, 6}, // Gamma Curve Related
    {0xed, {0x1b, 0x0b}, 2},
    {0xae, {0x77}, 1},
    {0xcd, {0x63}, 1},
    {0x70, {0x07, 0x07, 0x04, 0x0e, 0x0f, 0x09, 0x07, 0x08, 0x03}, 9},
    {0xe8, {0x04}, 1},
    {0x62, {0x18, 0x0d, 0x71, 0xed, 0x70, 0x70, 0x18, 0x0f, 0x71, 0xef, 0x70, 0x70}, 12},
    {0x63, {0x18, 0x11, 0x71, 0xf1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xf3, 0x70, 0x70}, 12},
    {0x64, {0x28, 0x29, 0xf1, 0x01, 0xf1, 0x00, 0x07}, 7},
    {0x66, {0x3c, 0x00, 0xcd, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}, 10},
    {0x67, {0x00, 0x3c, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}, 10},
    {0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4e, 0x00}, 7},
    {0x98, {0x3e, 0x07}, 2},
    {0x99, {0x3e, 0x07}, 2},
    {0x35, {0x00}, 1}, // Tearing Effect Line ON, V-blanking only
    {0x44, {0x00, 0x4a}, 2}, // Set Tear Scanline
    {0x21, {0x00}, 0}, // Display Inversion OFF
    {0x2a, {0x00, 0x00, 0x00, 0xef}, 4}, // Column Address Set (0 to 239)
    {0x2b, {0x00, 0x00, 0x00, 0xef}, 4}, // Row Address Set (0 to 239)
    {0x2c, {0x00}, 0}, // Memory Write
    {0x11, {0x00}, 0}, // Sleep Out
    {0x29, {0x00}, 0}, // Display ON
    {0, {0}, 0xff},    // End of commands
};

/**
 * @brief Initialize the GC9A01 LCD panel.
 *
 * This function sends a sequence of vendor-specific commands to initialize
 * the GC9A01 panel. This function is registered as part of the
 * esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel to be initialized.
 * @return
 *     - ESP_OK: Success
 */
static esp_err_t panel_gc9a01_init(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    // vTaskDelay(pdMS_TO_TICKS(100));
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
    //     gc9a01->madctl_val,
    // }, 1);
    int cmd = 0;
    while (vendor_specific_init[cmd].data_bytes != 0xff) {
        esp_lcd_panel_io_tx_param(io, vendor_specific_init[cmd].cmd, vendor_specific_init[cmd].data, vendor_specific_init[cmd].data_bytes & 0x1F);
        cmd++;
    }

    return ESP_OK;
}

/**
 * @brief Draw a bitmap on the GC9A01 LCD panel.
 *
 * This function sets the drawing window (column and row addresses) and
 * then sends the color data to the panel's GRAM. This function is
 * registered as part of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param x_start Starting X coordinate.
 * @param y_start Starting Y coordinate.
 * @param x_end Ending X coordinate (exclusive).
 * @param y_end Ending Y coordinate (exclusive).
 * @param color_data Pointer to the color data buffer.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = gc9a01->io;

    x_start += gc9a01->x_gap;
    x_end += gc9a01->x_gap;
    y_start += gc9a01->y_gap;
    y_end += gc9a01->y_gap;

    // define an area of frame memory where MCU can access
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * gc9a01->bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

/**
 * @brief Invert the colors of the GC9A01 LCD panel.
 *
 * This function sends the LCD_CMD_INVON or LCD_CMD_INVOFF command to
 * enable or disable color inversion. This function is registered as part
 * of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param invert_color_data True to invert colors, false to restore normal colors.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

/**
 * @brief Mirror the display of the GC9A01 LCD panel.
 *
 * This function updates the MADCTL register to mirror the display along
 * the X-axis and/or Y-axis. This function is registered as part of the
 * esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param mirror_x True to mirror along the X-axis, false otherwise.
 * @param mirror_y True to mirror along the Y-axis, false otherwise.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    if (mirror_x) {
        gc9a01->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        gc9a01->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9a01->madctl_val
    }, 1);
    return ESP_OK;
}

/**
 * @brief Swap the X and Y axes of the GC9A01 LCD panel.
 *
 * This function updates the MADCTL register to swap the X and Y axes,
 * effectively rotating the display. This function is registered as part
 * of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param swap_axes True to swap X and Y axes, false otherwise.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    if (swap_axes) {
        gc9a01->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9a01->madctl_val
    }, 1);
    return ESP_OK;
}

/**
 * @brief Set the X and Y gap (offset) for the GC9A01 LCD panel.
 *
 * This function sets an offset for the X and Y coordinates when drawing.
 * This can be used to adjust for display panels that are not perfectly
 * aligned or have a non-standard starting position. This function is
 * registered as part of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param x_gap The X offset.
 * @param y_gap The Y offset.
 * @return
 *     - ESP_OK: Success
 */
static esp_err_t panel_gc9a01_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    gc9a01->x_gap = x_gap;
    gc9a01->y_gap = y_gap;
    return ESP_OK;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
/**
 * @brief Turn the GC9A01 LCD panel display on or off.
 * (For ESP-IDF v5.0 and later)
 *
 * This function sends the LCD_CMD_DISPON or LCD_CMD_DISPOFF command to
 * turn the display on or off. This function is registered as part of the
 * esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param on_off True to turn the display on, false to turn it off.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    int command = 0;
    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}
#else // ESP_IDF_VERSION < 5.0.0
/**
 * @brief Turn the GC9A01 LCD panel display on or off.
 * (For ESP-IDF versions prior to v5.0)
 *
 * This function sends the LCD_CMD_DISPON or LCD_CMD_DISPOFF command to
 * turn the display on or off. The 'off' parameter is interpreted as
 * true for display off, false for display on. This function is registered
 * as part of the esp_lcd_panel_t structure.
 *
 * @param panel Handle to the panel.
 * @param off True to turn the display off, false to turn it on.
 * @return
 *     - ESP_OK: Success
 *     - Other ESP_ERR codes from lower-level IO functions.
 */
static esp_err_t panel_gc9a01_disp_off(esp_lcd_panel_t *panel, bool off)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    int command = 0;
    if (off) {
        command = LCD_CMD_DISPOFF;
    } else {
        command = LCD_CMD_DISPON;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}
#endif

