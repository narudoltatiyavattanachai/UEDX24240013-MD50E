# ESP32-C3 Round LCD Project Code Structure

This document provides a structural map and summary of the ESP32-C3 Round LCD project codebase.

## Project Structure

```
.
├── main/                                   # Main application code
│   ├── app_main.c                          # Entry point: LVGL config, NVS, brightness
│   ├── lvgl_port.c/h                       # LVGL display setup and task loop
│   └── ui/                                 # UI components
│       ├── ui.c/h                          # UI entry point, group management
│       ├── ui_menu.c/h                     # Application launcher menu
│       ├── ui_clock.c/h                    # Clock module
│       ├── ui_light.c/h                    # Light control module
│       ├── ui_fan.c/h                      # Fan speed module
│       ├── ui_player.c/h                   # Media player UI
│       ├── ui_washing.c/h                  # Washing machine UI
│       ├── ui_weather.c/h                  # Weather UI
│       ├── fonts/                          # Custom font resources
│       └── imgs/                           # Custom image resources
│
└── components/                             # External components
    └── bsp/                                # Board Support Package
        ├── bsp_indev.c/h                   # Input device handling (encoder)
        ├── bsp_lcd.c/h                     # LCD display interface
        └── lcd_panel_gc9a01.c/h            # GC9A01 LCD panel driver
```

## Key Functions

### LVGL Port and Task Management

- **`lvgl_port(lvgl_port_config_t *config)`**: Initialize LVGL, display, input devices, and create the LVGL task
- **`lvgl_task(void *arg)`**: **[HIGHLIGHTED]** Main LVGL task handler that processes LVGL timers and updates
- **`lvgl_sem_take(void)`**: Thread-safe mechanism to access LVGL APIs from other tasks
- **`lvgl_sem_give(void)`**: Release LVGL semaphore after accessing LVGL APIs

### UI Management

- **`ui_init(void)`**: **[HIGHLIGHTED]** Entry point for UI initialization, sets up encoder group and menu
- **`ui_add_obj_to_encoder_group(lv_obj_t *obj)`**: Add UI element to encoder group for rotation control
- **`ui_remove_all_objs_from_encoder_group(void)`**: Clear all objects from encoder group

### Module UI Initialization and Cleanup

- **`ui_menu_init(void)`**: Initialize main menu interface
- **`ui_clock_init(ret_cb_t ret_cb)`**: Initialize clock module with return callback
- **`ui_light_init(ret_cb_t ret_cb)`**: Initialize light control module with return callback
- **`ui_fan_init(ret_cb_t ret_cb)`**: Initialize fan control module with return callback
- **`ui_player_init(ret_cb_t ret_cb)`**: Initialize media player module with return callback
- **`ui_washing_init(ret_cb_t ret_cb)`**: Initialize washing machine module with return callback
- **`ui_weather_init(ret_cb_t ret_cb)`**: Initialize weather module with return callback

### BSP (Board Support Package)

- **`bsp_lcd_init(void)`**: Initialize LCD panel
- **`bsp_encoder_init(int gpio_a, int gpio_b)`**: Initialize rotary encoder input
- **`bsp_btn_init(int gpio_num)`**: Initialize button input

## Summary

This project implements a UI for controlling various home appliances and displaying information using an ESP32-C3 microcontroller with a round GC9A01 LCD display. The application uses LVGL for graphics rendering and provides a rotary encoder interface for navigation.

The code is organized into several modules:
- A main application entry point that initializes the system
- A LVGL port layer that handles the display and input device integration
- Multiple UI modules for different functionalities (clock, light, fan, player, weather, etc.)
- A BSP layer for hardware abstraction

**Note:** There is currently no UART implementation in this codebase, though it's mentioned in the project requirements.