/*
 * @file bsp_indev.c
 * @brief Board Support Package for input devices (encoder and button)
 *
 * This file provides the implementation for initializing and managing input
 * devices such as a rotary encoder and a push button. It handles GPIO
 * configuration, interrupt setup, and event processing for these devices.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"

#include "bsp_indev.h"

static const char *TAG = "bsp_indev";

// Queue to handle GPIO events from ISR
static xQueueHandle gpioEventQueue = NULL;

// Current value of the rotary encoder
static int32_t EC11_Value = 0;
// GPIO pin for encoder phase A
static int GPIO_CNT_A;
// GPIO pin for encoder phase B
static int GPIO_CNT_B;
// Current state of phase A
static bool pha_value = 0;
// Current state of phase B
static bool phb_value = 0;
// Previous state of phase A
static bool pha_value_old = 0;
// Previous state of phase B
static bool phb_value_old = 0;
// Flag indicating phase A changed
static bool pha_value_change = 0;
// Flag indicating phase B changed
static bool phb_value_change = 0;
// Unused time measurement variables
static int64_t begin_us = 0;
static int64_t end_us = 0;
static int64_t diff_us = 0;
// Direction of encoder rotation: 0 for none, 1 for increment, -1 for decrement
static size_t dir = 0;

// Array of callback functions for encoder events
static bsp_encoder_cb_t cbs[bsp_encoder_EVENT_MAX];
// Array of user data for encoder event callbacks
static void *cb_user_datas[bsp_encoder_EVENT_MAX];

/**
 * @brief Task to handle GPIO events for the rotary encoder.
 *
 * This task waits for GPIO events from the ISR, determines the encoder's
 * direction of rotation, updates the encoder value, and calls registered
 * callbacks.
 *
 * @param arg Unused.
 */
static void gpioTaskExample(void *arg)
{
    uint32_t ioNum = (uint32_t) arg;
    pha_value = gpio_get_level(GPIO_CNT_A);
    phb_value = gpio_get_level(GPIO_CNT_B);
    pha_value_old = pha_value;
    phb_value_old = phb_value;
    bsp_encoder_event_t event = bsp_encoder_EVENT_DEC;
    while (1) {
        if (xQueueReceive(gpioEventQueue, &ioNum, portMAX_DELAY)) {
            if (ioNum == GPIO_CNT_A) {
                pha_value = gpio_get_level(GPIO_CNT_A);
                if (pha_value != pha_value_old) {
                    pha_value_old = pha_value;
                    pha_value_change = 1;
                    if (phb_value_change != 1) {
                        if (pha_value_old != phb_value_old) {
                            dir = 1;
                            event = bsp_encoder_EVENT_INC;
                        } else {
                            dir = -1;
                            event = bsp_encoder_EVENT_DEC;
                        }
                    }
                }
            } else {
                phb_value = gpio_get_level(GPIO_CNT_B);
                if (phb_value != phb_value_old) {
                    phb_value_old = phb_value;
                    phb_value_change = 1;
                    if (pha_value_change != 1) {
                        if (pha_value_old != phb_value_old) {
                            dir = -1;
                            event = bsp_encoder_EVENT_DEC;
                        } else {
                            dir = 1;
                            event = bsp_encoder_EVENT_INC;
                        }
                    }
                }
            }
            if (pha_value_change == 1 && phb_value_change == 1 ) {
                EC11_Value += dir;
                if (cbs[event]) {
                    cbs[event](cb_user_datas[event]);
                }
                pha_value_change = 0;
                phb_value_change = 0;
                dir = 0;
            }
        }
    }
}

/**
 * @brief GPIO interrupt handler for the rotary encoder.
 *
 * This function is called when a GPIO interrupt occurs on either phase A or
 * phase B of the encoder. It sends the GPIO number to a queue to be
 * processed by the gpioTaskExample task.
 *
 * @param arg The GPIO number that triggered the interrupt.
 */
static void IRAM_ATTR intrHandler (void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpioEventQueue, &gpio_num, NULL);
}

/**
 * @brief Initializes the rotary encoder.
 *
 * This function configures the GPIO pins for phase A and phase B of the
 * encoder, sets up interrupt handlers, creates a queue for GPIO events,
 * and starts a task to process these events. This is part of the public API.
 *
 * @param gpio_a GPIO number for phase A of the encoder.
 * @param gpio_b GPIO number for phase B of the encoder.
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failure (though current implementation always returns ESP_OK)
 */
esp_err_t bsp_encoder_init(int gpio_a, int gpio_b)
{
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = BIT64(gpio_a) | BIT64(gpio_b),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
        .pull_up_en = 1,
    };
    gpio_config(&gpio_cfg);
    GPIO_CNT_A = gpio_a;
    GPIO_CNT_B = gpio_b;
    gpioEventQueue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpioTaskExample, "encoder", 2048, NULL, 10, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_CNT_A, intrHandler, (void *)GPIO_CNT_A);
    gpio_isr_handler_add(GPIO_CNT_B, intrHandler, (void *)GPIO_CNT_B);
    return ESP_OK;
}

/**
 * @brief Registers a callback function for an encoder event.
 *
 * This function allows application code to register a callback that will be
 * executed when a specific encoder event (increment or decrement) occurs.
 * This is part of the public API.
 *
 * @param event The encoder event to register for (bsp_encoder_EVENT_INC or bsp_encoder_EVENT_DEC).
 * @param cb The callback function to execute when the event occurs.
 * @param user_data User data to be passed to the callback function.
 * @return
 *     - ESP_OK: Success
 */
esp_err_t bsp_encoder_register_callback(bsp_encoder_event_t event, bsp_encoder_cb_t cb, void *user_data)
{
    cbs[event] = cb;
    cb_user_datas[event] = user_data;
    return ESP_OK;
}

/**
 * @brief Gets the current value of the rotary encoder.
 *
 * This function returns the accumulated value of the encoder, which is
 * incremented or decremented based on rotation. This is part of the public API.
 *
 * @return The current encoder value.
 */
int32_t bsp_encoder_get_value(void)
{
    return EC11_Value;
}

/**
 * @brief Initializes a button.
 *
 * This function configures the specified GPIO pin as an input for a button.
 * This is part of the public API.
 *
 * @param gpio_num GPIO number for the button.
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: If gpio_num is invalid.
 *     - ESP_FAIL: If gpio_config fails.
 */
esp_err_t bsp_btn_init(int gpio_num)
{
    ESP_RETURN_ON_FALSE(gpio_num != GPIO_NUM_NC, ESP_ERR_INVALID_ARG, TAG, "Invalid gpio_num");

    gpio_config_t config = {
        .pin_bit_mask = BIT64(gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "gpio config failed");

    return ESP_OK;
}

/**
 * @brief Gets the current state of a button.
 *
 * This function reads the current level of the specified GPIO pin to
 * determine the button's state (pressed or released). This is part of the public API.
 *
 * @param gpio_num GPIO number for the button.
 * @return The current state of the button (0 for low, 1 for high).
 */
uint8_t bsp_btn_get_state(int gpio_num)
{
    return gpio_get_level(gpio_num);
}
