/*
 * @file ui_menu.c
 * @brief Main menu UI implementation with a rotating icon interface
 *
 * This file implements the main menu of the user interface. It features
 * a visually appealing rotating icon menu where users can select different
 * applications or functions (e.g., Clock, Weather, Player). The menu
 * supports navigation via an encoder, with animations for icon transitions
 * and scaling effects to indicate focus. Each menu item is associated
 * with an initialization function that launches the respective UI screen.
 */
#include "lvgl.h"
#include <stdio.h>
#include "esp_system.h"
#ifdef ESP_IDF_VERSION
#include "esp_log.h"
#endif
#include "ui.h"
#include "ui_clock.h"
#include "ui_light.h"
#include "ui_player.h"
#include "ui_weather.h"
#include "ui_fan.h"
#include "ui_washing.h"

typedef struct {
    const char *name;
    const lv_img_dsc_t *icon;
    void (*create)(ret_cb_t ret_cb); /*!< Pointer to the function that creates the application's UI.
                                         It takes a `ret_cb_t` (return callback) as an argument,
                                         which is called when the application screen is exited. */
} ui_menu_app_t;

// LVGL image declarations for menu icons
LV_IMG_DECLARE(icon_clock);
LV_IMG_DECLARE(icon_fans);
LV_IMG_DECLARE(icon_light);
LV_IMG_DECLARE(icon_player);
LV_IMG_DECLARE(icon_weather);
LV_IMG_DECLARE(icon_washing);

static ui_menu_app_t menu[] = {
    {"clock", &icon_clock, ui_clock_init},
    {"washing", &icon_washing, ui_washing_init},
    {"fans", &icon_fans, ui_fan_init},
    {"light", &icon_light, ui_light_init},
    {"player", &icon_player, ui_player_init},
    {"weather", &icon_weather, ui_weather_init},
};

#define APP_NUM 5//(sizeof(menu) / sizeof(ui_menu_app_t))
#define APP_ICON_GAP_PIXEL (80)
#define ICONS_SHOW_NUM 3

static uint8_t app_index = 0;
static lv_obj_t *page;
static lv_obj_t *image_bg;
static bool anim_flag = false;
static lv_obj_t *icons[ICONS_SHOW_NUM + 1];
static lv_coord_t old_y[ICONS_SHOW_NUM + 1];
static uint8_t visible_index[ICONS_SHOW_NUM];
// Index of the icon that is currently not visible (used for animation)
static uint8_t invisable_index;

// Forward declaration for animation ready callback
static void menu_anim_ready_cb(lv_anim_t *a);

/**
 * @brief Calculates an index within a range, wrapping around with an offset.
 *
 * This utility function is used to calculate the new index of an application
 * in the `menu` array after a rotation (offset). It handles both positive
 * and negative offsets, ensuring the index wraps around correctly within
 * the bounds of `max`.
 *
 * @param num The current base number (e.g., current app index).
 * @param max The maximum value for the number (exclusive, typically the total number of apps).
 * @param offset The offset to apply (can be positive or negative).
 * @return The new number after applying the offset and wrapping around.
 *         Returns the original `num` if `num` is out of bounds (>= `max`).
 */
static uint32_t get_num_offset(uint32_t num, int32_t max, int32_t offset)
{
    if (num >= max) {
        printf("[ERROR] num should less than max\n");
        return num;
    }

    uint32_t i;
    if (offset >= 0) {
        i = (num + offset) % max;
    } else {
        offset = max + (offset % max);
        i = (num + offset) % max;
    }

    return i;
}

/**
 * @brief Gets the application index with a given offset from the current `app_index`.
 *
 * This function is a convenience wrapper around `get_num_offset` specifically
 * for calculating application indices in the main menu.
 *
 * @param offset The offset from the current `app_index` (e.g., -1 for previous, 1 for next).
 * @return The calculated application index.
 */
static uint32_t get_app_index(int8_t offset)
{
    return get_num_offset(app_index, APP_NUM, offset);
}

/**
 * @brief Callback function executed when returning from an application screen to the menu.
 *
 * This function is passed to each application's initialization function.
 * When the application screen is closed and control returns to the menu,
 * this callback re-adds the menu's background image object (`image_bg`)
 * to the encoder group, making the menu navigable again.
 *
 * @param args User arguments (not used in this implementation).
 */
static void app_return_cb(void *args)
{
    ui_add_obj_to_encoder_group(image_bg);
}

/**
 * @brief Animation execution callback for the menu icon scrolling.
 *
 * This function is called by LVGL during the menu scrolling animation.
 * It updates the Y position of each icon based on the animation value `v`.
 * It also applies a zoom effect to icons based on their proximity to the
 * center of the screen, making the central icon appear larger.
 *
 * @param args Pointer to user data, cast to `int8_t` representing the `extra_icon_index`
 *             which indicates the direction and target of the scroll.
 * @param v The current animation value (represents the amount of Y shift).
 */
static void menu_anim_exec_cb(void *args, int32_t v)
{
    int8_t extra_icon_index = (int8_t)args;

    for (int i = 0; i < ICONS_SHOW_NUM + 1; i++) {
        if (extra_icon_index > 0) {
            lv_obj_set_y(icons[i], old_y[i] - v);
        } else {
            lv_obj_set_y(icons[i], old_y[i] + v);
        }
        int32_t abs_y = LV_ABS(lv_obj_get_y_aligned(icons[i]));
        if (abs_y < 130) {
            lv_img_set_zoom(icons[i], 256 * (130 - abs_y) / 100);
        }
    }
}

/**
 * @brief Event callback for the main menu background image.
 *
 * This function handles events such as focus, key presses (encoder rotation),
 * and clicks on the menu's background image (`image_bg`), which acts as the
 * main interactive element for menu navigation.
 *
 * - `LV_EVENT_FOCUSED`: Enables editing mode for the LVGL group.
 * - `LV_EVENT_KEY`: Handles encoder left/right rotations. It determines the
 *   direction of scroll, sets up the next icon to be displayed, and starts
 *   the scrolling animation (`menu_anim_exec_cb`).
 * - `LV_EVENT_CLICKED`: Handles encoder button press. It disables group editing,
 *   removes all objects from the encoder group (so the launched app can
 *   add its own), and calls the `create` function of the currently selected
 *   application to launch its UI.
 *
 * @param e Pointer to the LVGL event object.
 */
static void menu_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    printf("evt=%d\n", code);
    if (LV_EVENT_FOCUSED == code) {
        lv_group_set_editing(lv_group_get_default(), true);
    } else if (LV_EVENT_KEY == code && false == anim_flag) {

        uint32_t key = lv_event_get_key(e);
        int8_t  extra_icon_index = 0;
        if (LV_KEY_RIGHT == key) {
            extra_icon_index = (ICONS_SHOW_NUM / 2) + 1;
        } else if (LV_KEY_LEFT == key) {
            extra_icon_index = -(ICONS_SHOW_NUM / 2) - 1;
        }

        lv_img_set_src(icons[invisable_index], menu[get_app_index(extra_icon_index)].icon);
        lv_obj_align(icons[invisable_index], LV_ALIGN_CENTER, 0, (extra_icon_index)* APP_ICON_GAP_PIXEL);
        lv_img_set_zoom(icons[invisable_index], 1);

        for (size_t i = 0; i < ICONS_SHOW_NUM + 1; i++) {
            old_y[i] = lv_obj_get_y_aligned(icons[i]);
        }

        anim_flag = true;
        lv_anim_t a1;
        lv_anim_init(&a1);
        lv_anim_set_var(&a1, (void *)extra_icon_index);
        lv_anim_set_values(&a1, 0, APP_ICON_GAP_PIXEL);
        lv_anim_set_exec_cb(&a1, menu_anim_exec_cb);
        lv_anim_set_path_cb(&a1, lv_anim_path_ease_in_out);
        lv_anim_set_ready_cb(&a1, menu_anim_ready_cb);
        lv_anim_set_time(&a1, 200);
        lv_anim_set_user_data(&a1, (void *)extra_icon_index);
        lv_anim_start(&a1);
    } else if (LV_EVENT_CLICKED == code) {
        lv_group_set_editing(lv_group_get_default(), false);
        ui_remove_all_objs_from_encoder_group();
        menu[get_app_index(0)].create(app_return_cb);
    }
}

/**
 * @brief Animation ready callback, executed when menu scrolling animation completes.
 *
 * This function is called by LVGL when the menu icon scrolling animation finishes.
 * It updates the `app_index` to reflect the new currently selected application.
 * It also updates the indices for `invisable_index` and `visible_index` arrays
 * to correctly position icons for the next potential animation. Finally, it
 * resets the `anim_flag` to allow new animations.
 *
 * @param a Pointer to the LVGL animation object.
 */
static void menu_anim_ready_cb(lv_anim_t *a)
{
    int8_t extra_icon_index = (int8_t)lv_anim_get_user_data(a);
    int8_t dir = extra_icon_index > 0 ? 1 : -1;
    app_index = get_app_index(dir);
    invisable_index = get_num_offset(invisable_index, ICONS_SHOW_NUM + 1, dir);
    for (size_t i = 0; i < ICONS_SHOW_NUM; i++) {
        visible_index[i] = get_num_offset(visible_index[i], ICONS_SHOW_NUM + 1, dir);
    }
    anim_flag = false;
    printf("dir=%d, app_index=%d, invisable_index=%d\n", dir, app_index, invisable_index);
}

/**
 * @brief Initializes the main menu UI.
 *
 * This function creates the main menu page, sets up the background image,
 * and initializes the display of application icons in a rotating list.
 * It positions the initial set of icons, applies zoom effects, and sets up
 * the invisible icon used for smooth animation transitions. Event callbacks
 * for menu navigation (focus, key press, click) are registered to the
 * background image object. The background image is then added to the
 * default encoder group to make it focusable.
 * This is part of the public API, typically called from `ui_init`.
 */
void ui_menu_init(void)
{
    if (page) {
        LV_LOG_WARN("menu page already created");
        return;
    }

    page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)));
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(page);
    lv_obj_refr_size(page);

    image_bg = lv_img_create(page);
    LV_IMG_DECLARE(img_bg);
    lv_img_set_src(image_bg, &img_bg);
    lv_obj_align(image_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_img_opa(image_bg, LV_OPA_60, 0);

    for (int i = 0; i < ICONS_SHOW_NUM; i++) {
        visible_index[i] = i;
        icons[visible_index[i]] = lv_img_create(image_bg);
        lv_img_set_src(icons[visible_index[i]], menu[get_app_index(i - (ICONS_SHOW_NUM / 2))].icon);
        lv_obj_align(icons[visible_index[i]], LV_ALIGN_CENTER, 0, (i - (ICONS_SHOW_NUM / 2)) * APP_ICON_GAP_PIXEL);
        int32_t abs_y = LV_ABS(lv_obj_get_y_aligned(icons[visible_index[i]]));
        if (abs_y < 130) {
            lv_img_set_zoom(icons[visible_index[i]], 256 * (130 - abs_y) / 100);
        } else {
            lv_img_set_zoom(icons[visible_index[i]], 1);
        }
    }
    invisable_index = ICONS_SHOW_NUM;
    icons[invisable_index] = lv_img_create(image_bg);
    lv_obj_set_size(icons[invisable_index], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(icons[invisable_index], LV_ALIGN_CENTER, 0, lv_obj_get_height(lv_obj_get_parent(image_bg)));


    lv_obj_add_event_cb(image_bg, menu_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(image_bg, menu_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(image_bg, menu_event_cb, LV_EVENT_CLICKED, NULL);
    ui_add_obj_to_encoder_group(image_bg);
}



