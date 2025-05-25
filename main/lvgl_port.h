/*
 * @file lvgl_port.h
 * @brief LVGL porting public API and configuration for ESP32
 *
 * This header file defines the public interface for the LVGL port on ESP32.
 * It includes the configuration structure `lvgl_port_config_t` used to
 * initialize the LVGL port, and prototypes for public functions that allow
 * other modules to interact with the LVGL system, such as semaphore
 * control and initialization.
 */
#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for LVGL port initialization.
 *
 * This structure holds all the necessary parameters to configure the LVGL
 * port, including display settings, tick period, and task parameters for
 * the LVGL handler.
 */
typedef struct {
    struct {
        uint16_t width;         /*!< Horizontal resolution of the display */
        uint16_t height;        /*!< Vertical resolution of the display */
        uint32_t buf_size;      /*!< Size of the LVGL draw buffer(s) in number of pixels */
        int buf_caps;           /*!< Memory capabilities for allocating draw buffer(s) (e.g., MALLOC_CAP_DMA) */
    } display;                  /*!< Display configuration settings */
    uint8_t tick_period;        /*!< Period of the LVGL tick in milliseconds */
    struct {
        uint8_t period;         /*!< Period for calling lv_timer_handler() in the LVGL task, in milliseconds */
        uint8_t core_id;        /*!< CPU core ID where the LVGL task will be pinned (0 or 1, or tskNO_AFFINITY) */
        int priority;           /*!< Priority of the LVGL task */
    } task;                     /*!< LVGL task configuration settings */
    bool avoid_tear;            /*!< Set to true to enable tearing effect avoidance (waits for TE signal if available) */
} lvgl_port_config_t;

/**
 * @brief Takes the LVGL semaphore.
 *
 * This function is used to protect LVGL API calls from concurrent access.
 * It should be called before any LVGL function if the calling task is not
 * the main LVGL task. This is part of the public API.
 */
void lvgl_sem_take(void);

/**
 * @brief Gives the LVGL semaphore.
 *
 * This function releases the LVGL semaphore, allowing other tasks to access
 * LVGL APIs. It should be called after an LVGL function call if the calling
 * task is not the main LVGL task. This is part of the public API.
 */
void lvgl_sem_give(void);

/**
 * @brief Initializes the LVGL port.
 *
 * This function initializes LVGL itself, the display, input devices, and the
 * LVGL tick. It also creates a FreeRTOS task to handle LVGL events.
 * This is part of the public API.
 *
 * @param config Pointer to the LVGL port configuration structure.
 *               The contents of this structure are used to set up the LVGL port.
 */
void lvgl_port(lvgl_port_config_t *config);

#ifdef __cplusplus
}
#endif

#endif