/*
 * @file lvgl_port.c
 * @brief LVGL porting implementation for ESP32
 *
 * This file provides the necessary functions to integrate the LVGL library
 * with the ESP32 platform. It handles display initialization, input device
 * (encoder and button) registration, LVGL tick generation, and task management
 * for LVGL event handling. It also includes semaphore protection for LVGL
 * function calls from different tasks.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include "lvgl.h"
#include "bsp_lcd.h"
#include "bsp_indev.h"
#include "lvgl_port.h"

static char *TAG = "lvgl_port";
// LVGL display driver
static lv_disp_drv_t disp_drv;
// LVGL input device driver (encoder)
static lv_indev_drv_t indev_drv;
// Handle for the LVGL task
static TaskHandle_t task = NULL;
// Semaphore for protecting LVGL API calls
static SemaphoreHandle_t sem_lock = NULL;

// Forward declarations for static functions
static void display_init(lvgl_port_config_t *config);
static void tick_init(uint8_t period);
static void lvgl_task(void *arg);

/**
 * @brief Takes the LVGL semaphore.
 *
 * This function is used to protect LVGL API calls from concurrent access.
 * It should be called before any LVGL function if the calling task is not
 * the main LVGL task. This is part of the public API.
 */
void lvgl_sem_take(void)
{
    if (xTaskGetCurrentTaskHandle() != task) {
        xSemaphoreTake(sem_lock, portMAX_DELAY);
    }
}

/**
 * @brief Gives the LVGL semaphore.
 *
 * This function releases the LVGL semaphore, allowing other tasks to access
 * LVGL APIs. It should be called after an LVGL function call if the calling
 * task is not the main LVGL task. This is part of the public API.
 */
void lvgl_sem_give(void)
{
    if (xTaskGetCurrentTaskHandle() != task) {
        xSemaphoreGive(sem_lock);
    }
}

/**
 * @brief Reads the encoder state for LVGL.
 *
 * This function is registered as the read callback for the LVGL input device
 * driver. It reads the encoder's accumulated value and the state of its
 * associated button, then populates the `lv_indev_data_t` structure.
 *
 * @param indev_drv Pointer to the LVGL input device driver.
 * @param data Pointer to the `lv_indev_data_t` structure to store the read data.
 */
static void encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static int32_t last_v = 0;
    static int32_t invd = 0;

    invd = bsp_encoder_get_value();
    data->enc_diff = last_v - invd;
    data->state = (bsp_btn_get_state(BSP_BTN_PIN_NUM) == 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    last_v = invd;
}

/**
 * @brief Initializes the input device (encoder and button) for LVGL.
 *
 * This function initializes the BSP for the encoder and button, then
 * registers an LVGL input device driver for the encoder.
 */
static void indev_init(void)
{
    bsp_encoder_init(BSP_ENCODER_A_PIN_NUM, BSP_ENCODER_B_PIN_NUM);
    bsp_btn_init(BSP_BTN_PIN_NUM);

    /*Register a encoder input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    lv_indev_drv_register(&indev_drv);
}

/**
 * @brief Initializes the LVGL port.
 *
 * This function initializes LVGL itself, the display, input devices, and the
 * LVGL tick. It also creates a FreeRTOS task to handle LVGL events.
 * This is part of the public API.
 *
 * @param config Pointer to the LVGL port configuration structure.
 */
void lvgl_port(lvgl_port_config_t *config)
{
    lv_init();
    display_init(config);
    indev_init();
    tick_init(config->tick_period);

    sem_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(sem_lock);
    xTaskCreatePinnedToCore(
        lvgl_task, "lvgl", 4096, (void *)config->task.period, config->task.priority,
        &task, config->task.core_id
    );
    ESP_LOGI(TAG, "Finish init");
}

/**
 * @brief LVGL display flush callback.
 *
 * This function is registered as the `flush_cb` for the LVGL display driver.
 * It is called by LVGL when a region of the display needs to be redrawn.
 * It uses the BSP LCD functions to draw the bitmap to the display.
 * If `full_refresh` is enabled in the display driver, it waits for the
 * `flush_ready` signal from the TE line (if configured).
 *
 * @param drv Pointer to the LVGL display driver.
 * @param area Pointer to the area to be flushed.
 * @param color_p Pointer to the color data buffer.
 */
static void flush_cb(struct _lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    if (drv->full_refresh) {
        bsp_lcd_wait_flush_ready();
    }
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_p);
}

/**
 * @brief Callback for LCD transaction done.
 *
 * This function is registered with the BSP LCD module. It is called when an
 * LCD transaction (e.g., drawing a bitmap) is complete. It signals LVGL
 * that the flush operation is ready.
 *
 * @param args User arguments (not used in this implementation).
 * @return true to indicate that a higher priority task has been woken.
 */
static bool trans_done_cb(void *args)
{
    lv_disp_flush_ready(&disp_drv);
    return true;
}

/**
 * @brief Initializes the display for LVGL.
 *
 * This function initializes the BSP LCD, allocates display buffers (draw buffers)
 * for LVGL, and registers the LVGL display driver with the appropriate
 * callbacks and settings.
 *
 * @param config Pointer to the LVGL port configuration structure.
 */
static void display_init(lvgl_port_config_t *config)
{
    esp_lcd_panel_handle_t panel_handle = bsp_lcd_init();

    static lv_disp_draw_buf_t disp_buf;
    lv_color_t *buf_1 = (lv_color_t *)heap_caps_malloc(config->display.buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf_2 = (lv_color_t *)heap_caps_malloc(config->display.buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, config->display.buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = flush_cb;
    disp_drv.hor_res = config->display.width;
    disp_drv.ver_res = config->display.height;
    disp_drv.full_refresh = (config->avoid_tear) ? 1 : 0;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);
    bsp_lcd_trans_done_cb_register(trans_done_cb);
}

/**
 * @brief LVGL tick increment function.
 *
 * This function is called periodically by an ESP timer to increment the LVGL
 * tick counter.
 *
 * @param arg The period of the tick in milliseconds, passed as a void pointer.
 */
static void tick_inc(void *arg)
{
    uint8_t period = (uint8_t)arg;
    lv_tick_inc(period);
}

/**
 * @brief Initializes the LVGL tick system.
 *
 * This function creates and starts an ESP timer that periodically calls
 * `tick_inc` to provide a time base for LVGL.
 *
 * @param period The period of the LVGL tick in milliseconds.
 */
static void tick_init(uint8_t period)
{
    esp_timer_handle_t timer;
    esp_timer_create_args_t args = {
        .name = "lvgl_tick",
        .callback = tick_inc,
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = true,
        .arg = (void *)period,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, period * 1000));
}

/**
 * @brief Main LVGL task.
 *
 * This FreeRTOS task is responsible for calling `lv_timer_handler()`
 * periodically. This function handles LVGL's internal tasks, such as
 * animation and event processing. The task is protected by a semaphore
 * to ensure thread-safe access to LVGL APIs.
 *
 * @param arg The period for calling `lv_timer_handler()` in milliseconds,
 *            passed as a void pointer.
 */
static void lvgl_task(void *arg)
{
    uint8_t period = (uint8_t)arg;
    for (;;) {
        xSemaphoreTake(sem_lock, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(sem_lock);
        vTaskDelay(pdMS_TO_TICKS(period));
    }
}
