# API Reference

This document provides a comprehensive reference of all public APIs in the ESP32-C3 Round LCD project.

## LVGL Port APIs

| Function | Defined in | Purpose |
|----------|------------|---------|
| `void lvgl_port(lvgl_port_config_t *config)` | lvgl_port.h | Initialize LVGL, display, input devices, and create the LVGL task |
| `void lvgl_sem_take(void)` | lvgl_port.h | Take semaphore to safely access LVGL APIs from tasks other than lvgl_task |
| `void lvgl_sem_give(void)` | lvgl_port.h | Release semaphore after accessing LVGL APIs from other tasks |

## UI Core APIs

| Function | Defined in | Purpose |
|----------|------------|---------|
| `void ui_init(void)` | ui/ui.h | Initialize UI system, create encoder group, and start main menu |
| `void ui_add_obj_to_encoder_group(lv_obj_t *obj)` | ui/ui.h | Add a UI object to encoder group for input control |
| `void ui_remove_all_objs_from_encoder_group(void)` | ui/ui.h | Remove all objects from encoder group |

## UI Module APIs

| Function | Defined in | Purpose |
|----------|------------|---------|
| `void ui_menu_init(void)` | ui/ui_menu.h | Initialize main menu interface |
| `void ui_menu_delete(void)` | ui/ui_menu.h | Clean up and delete main menu |
| `void ui_clock_init(ret_cb_t ret_cb)` | ui/ui_clock.h | Initialize clock module with return callback |
| `void ui_clock_delete(void)` | ui/ui_clock.h | Clean up and delete clock UI |
| `void ui_light_init(ret_cb_t ret_cb)` | ui/ui_light.h | Initialize light control module with return callback |
| `void ui_light_delete(void)` | ui/ui_light.h | Clean up and delete light control UI |
| `void ui_fan_init(ret_cb_t ret_cb)` | ui/ui_fan.h | Initialize fan control module with return callback |
| `void ui_fan_delete(void)` | ui/ui_fan.h | Clean up and delete fan control UI |
| `void ui_player_init(ret_cb_t ret_cb)` | ui/ui_player.h | Initialize media player module with return callback |
| `void ui_player_delete(void)` | ui/ui_player.h | Clean up and delete media player UI |
| `void ui_washing_init(ret_cb_t ret_cb)` | ui/ui_washing.h | Initialize washing machine module with return callback |
| `void ui_washing_delete(void)` | ui/ui_washing.h | Clean up and delete washing machine UI |
| `void ui_weather_init(ret_cb_t ret_cb)` | ui/ui_weather.h | Initialize weather module with return callback |
| `void ui_weather_delete(void)` | ui/ui_weather.h | Clean up and delete weather UI |

## BSP LCD APIs

| Function | Defined in | Purpose |
|----------|------------|---------|
| `esp_lcd_panel_handle_t bsp_lcd_init(void)` | bsp/bsp_lcd.h | Initialize LCD panel and return handle |
| `void bsp_lcd_trans_done_cb_register(bsp_lcd_trans_done_cb_t callback)` | bsp/bsp_lcd.h | Register callback for LCD transfer completion |
| `void bsp_lcd_set_brightness(uint8_t percent)` | bsp/bsp_lcd.h | Set LCD backlight brightness (0-100%) |
| `void bsp_lcd_wait_flush_ready(void)` | bsp/bsp_lcd.h | Block until LCD flush operation is complete |

## BSP Input Device APIs

| Function | Defined in | Purpose |
|----------|------------|---------|
| `esp_err_t bsp_encoder_init(int gpio_a, int gpio_b)` | bsp/bsp_indev.h | Initialize rotary encoder on specified GPIO pins |
| `int32_t bsp_encoder_get_value(void)` | bsp/bsp_indev.h | Get current encoder position value |
| `esp_err_t bsp_encoder_register_callback(bsp_encoder_event_t event, bsp_encoder_cb_t cb, void *user_data)` | bsp/bsp_indev.h | Register callback for encoder events |
| `esp_err_t bsp_btn_init(int gpio_num)` | bsp/bsp_indev.h | Initialize button on specified GPIO pin |
| `uint8_t bsp_btn_get_state(int gpio_num)` | bsp/bsp_indev.h | Get current button state (pressed/released) |

## GC9A01 LCD Panel API

| Function | Defined in | Purpose |
|----------|------------|---------|
| `esp_err_t lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)` | bsp/lcd_panel_gc9a01.h | Create and initialize a GC9A01 LCD panel driver |

## Custom Type Definitions

| Type | Defined in | Purpose |
|------|------------|---------|
| `typedef void (*ret_cb_t)(void *args)` | ui/ui.h | Return callback function type used across UI modules |
| `typedef bool (*bsp_lcd_trans_done_cb_t)(void)` | bsp/bsp_lcd.h | LCD transfer completion callback function type |
| `typedef void (* bsp_encoder_cb_t)(void *)` | bsp/bsp_indev.h | Encoder event callback function type |