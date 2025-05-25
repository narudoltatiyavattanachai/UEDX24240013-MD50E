/*
 * @file bsp_indev.h
 * @brief Board Support Package for input devices (encoder and button) - Public API
 *
 * This header file defines the public interface for interacting with input
 * devices like rotary encoders and buttons. It includes type definitions for
 * events and callbacks, and function prototypes for initializing and using
 * these devices.
 */
#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

// Default GPIO pin number for the button
#define BSP_BTN_PIN_NUM         (GPIO_NUM_9)
// Default GPIO pin number for encoder phase A
#define BSP_ENCODER_A_PIN_NUM   (GPIO_NUM_7)
// Default GPIO pin number for encoder phase B
#define BSP_ENCODER_B_PIN_NUM   (GPIO_NUM_6)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration of possible encoder events.
 */
typedef enum {
    bsp_encoder_EVENT_INC, /*!< Encoder value increased */
    bsp_encoder_EVENT_DEC, /*!< Encoder value decreased */
    bsp_encoder_EVENT_MAX, /*!< Maximum number of encoder events */
} bsp_encoder_event_t;

/**
 * @brief Callback function type for encoder events.
 *
 * @param user_data User data passed to the callback.
 */
typedef void (* bsp_encoder_cb_t)(void *);

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
esp_err_t bsp_encoder_init(int gpio_a, int gpio_b);

/**
 * @brief Gets the current value of the rotary encoder.
 *
 * This function returns the accumulated value of the encoder, which is
 * incremented or decremented based on rotation. This is part of the public API.
 *
 * @return The current encoder value.
 */
int32_t bsp_encoder_get_value(void);

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
esp_err_t bsp_encoder_register_callback(bsp_encoder_event_t event, bsp_encoder_cb_t cb, void *user_data);

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
esp_err_t bsp_btn_init(int gpio_num);

/**
 * @brief Gets the current state of a button.
 *
 * This function reads the current level of the specified GPIO pin to
 * determine the button's state (pressed or released). This is part of the public API.
 *
 * @param gpio_num GPIO number for the button.
 * @return The current state of the button (0 for low, 1 for high).
 */
uint8_t bsp_btn_get_state(int gpio_num);

#ifdef __cplusplus
}
#endif
