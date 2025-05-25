/*
 * @file app_main.c
 * @brief Main application entry point for the ESP32-based LVGL UI project.
 *
 * This file contains the primary `app_main` function that initializes the
 * system, including NVS, LVGL, the display, input devices, and the user
 * interface. It also includes optional memory and task monitoring utilities.
 *
 * Original Example Note (for context, not part of this project's direct purpose):
 * Hello World Example
 * This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "bsp_lcd.h"
#include "lvgl_port.h"
#include "ui/ui.h"

static const char *TAG = "main";


#define MEMORY_MONITOR 1 // Switch to enable/disable memory and task monitoring

#if MEMORY_MONITOR

#define ARRAY_SIZE_OFFSET   5   // Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

/**
 * @brief Prints real-time CPU usage statistics for FreeRTOS tasks.
 *
 * This function measures and prints the CPU usage of tasks over a specified
 * duration (`xTicksToWait`). It calls `uxTaskGetSystemState()` twice, separated
 * by the delay, and then calculates the differences in task run times to
 * determine CPU usage percentages.
 *
 * @note If tasks are added or removed during the measurement period, their
 *       stats might not be accurately reported or printed as "deleted" or "created".
 * @note This function should ideally be called from a high-priority task to
 *       minimize measurement inaccuracies caused by its own execution delay.
 * @note In dual-core mode, CPU usage is reported per core (e.g., 50% represents
 *       full utilization of one core).
 *
 * @param xTicksToWait The period, in FreeRTOS ticks, over which to measure task statistics.
 *
 * @return
 *  - ESP_OK: Successfully printed the statistics.
 *  - ESP_ERR_NO_MEM: Insufficient memory to allocate internal arrays for task statuses.
 *  - ESP_ERR_INVALID_SIZE: Insufficient array size provided to `uxTaskGetSystemState()`.
 *    Try increasing `ARRAY_SIZE_OFFSET`.
 *  - ESP_ERR_INVALID_STATE: The delay duration `xTicksToWait` was too short,
 *    resulting in no measurable time difference.
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret;

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    printf("| Task \t\t| Run Time \t| Percentage\n");
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        int k = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                k = j;
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        //Check if matching task found
        if (k >= 0) {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            printf("| %s \t\t| %d \t| %d%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            printf("| %s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            printf("| %s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}

/**
 * @brief FreeRTOS task for monitoring system resources.
 *
 * This task periodically prints system memory usage (free internal RAM,
 * free SPIRAM, largest free blocks, minimum ever free sizes) and
 * real-time task CPU utilization statistics (using `print_real_time_stats`).
 * The monitoring interval is defined by `STATS_TICKS`.
 *
 * @param arg Unused task parameter.
 */
static void monitor_task(void *arg)
{
    (void) arg;
    const int STATS_TICKS = pdMS_TO_TICKS(2 * 1000);

    while (true) {
        ESP_LOGI(TAG, "System Info Trace");
        printf("\tDescription\tInternal\tSPIRAM\n");
        printf("Current Free Memory\t%d\t\t%d\n",
               heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        printf("Largest Free Block\t%d\t\t%d\n",
               heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        printf("Min. Ever Free Size\t%d\t\t%d\n",
               heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));

        printf("Getting real time stats over %d ticks\n", STATS_TICKS);
        if (print_real_time_stats(STATS_TICKS) == ESP_OK) {
            printf("Real time stats obtained\n");
        } else {
            printf("Error getting real time stats\n");
        }

        vTaskDelay(STATS_TICKS);
    }

    vTaskDelete(NULL);
}

/**
 * @brief Starts the system monitoring task.
 *
 * This function creates and pins the `monitor_task` to a specific core (core 0).
 * The task is given a high priority (configMAX_PRIORITIES - 3).
 * It checks for successful task creation.
 */
static void sys_monitor_start(void)
{
    BaseType_t ret_val = xTaskCreatePinnedToCore(monitor_task, "Monitor Task", 4 * 1024, NULL, configMAX_PRIORITIES - 3, NULL, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
}
#endif // MEMORY_MONITOR

/**
 * @brief Main application entry point.
 *
 * This is the first function called after the FreeRTOS scheduler starts.
 * It performs the following key steps:
 * 1. Initializes the Non-Volatile Storage (NVS) flash memory, erasing it
 *    if it's corrupted or a new version is found.
 * 2. Configures and initializes the LVGL port, including display settings,
 *    tick period, and LVGL task parameters.
 * 3. If `MEMORY_MONITOR` is enabled, it starts the system monitoring task.
 * 4. Initializes the main User Interface (UI) by calling `ui_init()`. This
 *    call is protected by LVGL semaphores (`lvgl_sem_take`/`lvgl_sem_give`)
 *    to ensure thread safety if other tasks were to interact with LVGL
 *    concurrently (though not typical at this stage of init).
 * 5. Sets the LCD backlight brightness to 100% after a short delay.
 *
 * @note This function does not return.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Compile time: %s %s", __DATE__, __TIME__);
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    lvgl_port_config_t lvgl_config = {
        .display = {
            .width = LCD_H_RES,
            .height = LCD_V_RES,
            .buf_size = LCD_H_RES * LCD_V_RES,
        },
        .tick_period = 2,
        .task = {
            .core_id = 0,
            .period = 5,
            .priority = 5,
        },
        .avoid_tear = true,
    };
    lvgl_port(&lvgl_config);

#if MEMORY_MONITOR
    sys_monitor_start();
#endif  

    lvgl_sem_take();
    ui_init();
    lvgl_sem_give();

    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_lcd_set_brightness(100);
}
