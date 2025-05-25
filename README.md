# Embedded Application with LVGL on ESP32

## Overview

This project is an embedded application for an ESP32 microcontroller, featuring a User Interface (UI) built with the Light and Versatile Graphics Library (LVGL). It showcases various UI screens such as an analog clock, fan controller, music player, washing machine interface, and weather display, all navigable via a rotary encoder. The project is structured with a clear separation between hardware abstraction, LVGL porting, core UI logic, and individual UI modules.

## Directory Structure

*   **`components/bsp/`**: Board Support Package
    *   Contains hardware-specific drivers and an abstraction layer for the target board's peripherals, including the LCD display and input devices (rotary encoder, buttons). This isolates higher-level application code from direct hardware manipulation.
*   **`main/`**: Main Application and Core Logic
    *   Houses the primary application entry point (`app_main.c`), the LVGL porting layer (`lvgl_port.c/h`), and the user interface logic (within the `ui/` subdirectory).
*   **`main/ui/`**: User Interface Logic and Components
    *   Contains the core UI management code (`ui.c/h`), the main application menu (`ui_menu.c/h`), and all individual UI screen modules (e.g., `ui_clock.c/h`, `ui_fan.c/h`).
*   **`main/ui/fonts/`**: UI Fonts
    *   Stores custom font files (converted to C arrays) used by LVGL to render text in the UI.
*   **`main/ui/imgs/`**: UI Images
    *   Stores image assets (converted to C arrays) such as icons and backgrounds used throughout the UI.

## Software Architecture/Components

The application is layered to promote modularity and ease of maintenance:

*   **BSP (Board Support Package)** (`components/bsp/`)
    *   **Role:** Provides a hardware abstraction layer (HAL) for direct interaction with the ESP32's peripherals and any connected external components like the display and encoder.
    *   **Key Functions/Modules:**
        *   `bsp_lcd.c/h`: Manages the LCD. Key public API functions include `bsp_lcd_init()` for display setup, `bsp_lcd_set_brightness()` for backlight control, and `bsp_lcd_trans_done_cb_register()` for display transaction callbacks. It utilizes a specific panel driver like `lcd_panel_gc9a01.c/h` (whose main API is `lcd_new_panel_gc9a01()`).
        *   `bsp_indev.c/h`: Manages input devices. Key public API functions include `bsp_encoder_init()` and `bsp_btn_init()` for setting up the rotary encoder and button(s), `bsp_encoder_get_value()` to read encoder counts, and `bsp_btn_get_state()` for button status.

*   **LVGL Port (`main/lvgl_port.c/h`)**
    *   **Role:** Acts as the bridge between the generic LVGL graphics library and the specific hardware/BSP of the ESP32 platform.
    *   It initializes LVGL itself (`lv_init()`).
    *   It configures and registers LVGL display drivers, using `bsp_lcd_init()` from the BSP for low-level LCD operations. A `flush_cb` callback is implemented to send rendered pixel data to the display via the BSP and underlying ESP-IDF SPI/LCD drivers.
    *   It configures and registers LVGL input device drivers (specifically for an encoder in this project), using `bsp_encoder_init()` and `bsp_btn_init()`. A `read_cb` callback periodically polls the input device state via `bsp_encoder_get_value()` and `bsp_btn_get_state()` and translates this into LVGL input events.
    *   It sets up the LVGL system tick using an ESP timer.
    *   It creates a dedicated FreeRTOS task (`lvgl_task`) to run `lv_timer_handler()`, which processes LVGL's internal tasks like animations and event dispatching.
    *   **Key Public API:** `lvgl_port(lvgl_port_config_t *config)` for initialization, and `lvgl_sem_take()` / `lvgl_sem_give()` for thread-safe access to LVGL functions from other tasks.

*   **UI Layer (`main/ui/`)**
    *   **Core UI (`ui.c/h`)**:
        *   **Role:** Manages the overall UI structure, global UI elements, and navigation, particularly focusing on encoder-based input. It initializes the main application menu and provides common utilities for UI screens.
        *   **Key Public API:** `ui_init()` to start the UI, `ui_add_obj_to_encoder_group(lv_obj_t *obj)` to make UI elements navigable by the encoder, and `ui_remove_all_objs_from_encoder_group(void)` to clear the focus group (useful when switching screens). It also defines `ret_cb_t`, a common callback type for UI screens to signal their completion.

    *   **UI Screens/Modules (`ui_*.c/h`, e.g., `ui_clock.c/h`, `ui_menu.c/h`)**:
        *   **Role:** Each module implements a specific screen or application within the UI (e.g., clock display, fan control, main menu).
        *   **Structure:** Typically, each screen module has:
            *   An `_init(ret_cb_t callback)` function: This function is responsible for creating all LVGL objects (widgets, labels, images, etc.) for that screen and registering event callbacks for its interactive elements. It saves the `ret_cb_t` to be called upon exiting the screen.
            *   A `_delete(void)` function: This function cleans up all LVGL objects created by the `_init` function, frees any allocated resources, and then calls the saved `ret_cb_t` to return control to the calling context (usually the main menu).
        *   **Management:** UI screens are generally launched from the `ui_menu.c` module. When a screen is exited, it uses the `ret_cb_t` to call back to `ui_menu.c`, which then re-enables interaction with the menu.

*   **Main Application (`main/app_main.c`)**
    *   **Role:** Serves as the entry point for the firmware (`app_main` function). It is responsible for initializing the entire system in the correct order: NVS, LVGL port, BSP components (indirectly via LVGL port), and finally the main user interface. It can also set global system parameters like initial LCD brightness.

## Key API Linkages and Call Flow

1.  **Initialization Sequence:**
    *   The application starts with `app_main()` in `main/app_main.c`.
    *   `app_main()` first initializes system-level components like NVS (Non-Volatile Storage).
    *   It then calls `lvgl_port(&lvgl_config)`. This function:
        *   Initializes LVGL itself.
        *   Calls `bsp_lcd_init()` (from `components/bsp/bsp_lcd.c`) to set up the LCD hardware.
        *   Calls `bsp_encoder_init()` and `bsp_btn_init()` (from `components/bsp/bsp_indev.c`) to set up input devices.
        *   Registers the LVGL display and input drivers, linking LVGL's drawing and input handling to the BSP functions.
        *   Starts the LVGL tick timer and the main LVGL FreeRTOS task.
    *   After `lvgl_port()` returns, `app_main()` calls `ui_init()` (from `main/ui/ui.c`).
    *   `ui_init()` in turn calls `ui_menu_init()` (from `main/ui/ui_menu.c`) to create and display the main application selection menu.
    *   `app_main()` may then perform final setup like `bsp_lcd_set_brightness()`.

2.  **UI Screen Loading and Transitions:**
    *   The `ui_menu.c` module acts as the central navigator. It typically displays a list of available UI screens/applications.
    *   When a user selects an item from the menu (e.g., "Clock"):
        *   The menu's event handler calls `ui_remove_all_objs_from_encoder_group()` to remove menu items from the focus group.
        *   It then calls the specific screen's initialization function, for example, `ui_clock_init(app_return_cb)`. The `app_return_cb` is a static function within `ui_menu.c` that will be used to return to the menu.
    *   The launched screen (e.g., `ui_clock.c`):
        *   Creates its LVGL widgets.
        *   Adds its interactive elements to the default encoder group using `ui_add_obj_to_encoder_group()` so they can be focused and controlled by the encoder.
    *   When the user action triggers an exit from the current screen (e.g., a long press):
        *   The screen's event handler calls its own `_delete()` function (e.g., `ui_clock_delete()`).
        *   The `_delete()` function frees all LVGL objects of that screen, stops any related timers or animations, and then calls the `return_callback` (which is `app_return_cb` from `ui_menu.c`).
    *   The `app_return_cb` in `ui_menu.c` then adds the menu's main interactive element back to the encoder group (`ui_add_obj_to_encoder_group(image_bg)`), making the menu navigable again.

3.  **Input Event Handling:**
    *   The rotary encoder/button state is read by the `read_cb` (e.g., `encoder_read` in `lvgl_port.c`) using BSP functions like `bsp_encoder_get_value()` and `bsp_btn_get_state()`.
    *   This data is passed to LVGL's input device processing.
    *   LVGL translates these low-level inputs into higher-level events (e.g., `LV_EVENT_KEY`, `LV_EVENT_CLICKED`, group navigation events).
    *   These LVGL events are then dispatched to the event callback function of the currently focused LVGL object within the active UI screen. For example, an encoder turn might result in `LV_KEY_LEFT` or `LV_KEY_RIGHT`, which is handled by `menu_event_cb` in `ui_menu.c` to scroll through menu items, or by an arc widget's event handler in `ui_light.c` to change brightness.

## How to Build

This project is based on the ESP-IDF framework.
1.  Ensure you have the ESP-IDF environment set up correctly.
2.  Navigate to the project's root directory.
3.  Build the project using the command: `idf.py build`
4.  Flash the project to the ESP32: `idf.py -p (PORT) flash` (replace `(PORT)` with your ESP32's serial port).
5.  Monitor the output: `idf.py -p (PORT) monitor`.
