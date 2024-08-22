
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include "status_bar.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_status_state_bar {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

struct output_status_state_bar {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static void draw_status_bar(struct zmk_widget_status_bar *widget) {
    lv_canvas_set_buffer(widget->canvas, widget->cbuf, CANVAS_SIZE, STATUS_BAR_HEIGHT,
                         LV_IMG_CF_TRUE_COLOR);
    // lv_canvas_draw_rect(widget->obj, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(widget->canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    lv_canvas_draw_text(widget->canvas, 0, 20, 68, &widget->battery_label_dsc,
                        widget->battery_text);
    lv_canvas_draw_text(widget->canvas, 0, 0, 68, &widget->output_label_dsc, widget->output_text);

    for (int i = 0; i < CANVAS_SIZE; i++) {
        for (int j = 0; j < STATUS_BAR_HEIGHT; j++) {
            widget->cbuf_tmp[(CANVAS_SIZE - i - 1) * STATUS_BAR_HEIGHT + j] =
                widget->cbuf[i + j * CANVAS_SIZE];
        }
    }

    // memcpy(widget->cbuf_tmp, widget->cbuf, sizeof(widget->cbuf_tmp));
    // widget->img.data = (void *)widget->cbuf_tmp;
    lv_canvas_set_buffer(widget->canvas, widget->cbuf_tmp, STATUS_BAR_HEIGHT, CANVAS_SIZE,
                         LV_IMG_CF_TRUE_COLOR);
    // lv_canvas_transform(widget->canvas, &widget->img, widget->angle, LV_IMG_ZOOM_NONE, 0, 0,
    //                     CANVAS_SIZE / 2, CANVAS_SIZE / 2, true);
}

static void set_battery_symbol_bar(struct zmk_widget_status_bar *widget,
                                   struct battery_status_state_bar state) {
    strcpy(widget->battery_text, "");

    uint8_t level = state.level;

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (state.usb_present) {
        strcpy(widget->battery_text, LV_SYMBOL_CHARGE " ");
    }
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS_SHOW_PERCENTAGE)
    char perc[5] = {};
    snprintf(perc, sizeof(perc), "%3u%%", level);
    strcat(widget->battery_text, perc);
#else
    if (level > 95) {
        strcat(widget->battery_text, LV_SYMBOL_BATTERY_FULL);
    } else if (level > 65) {
        strcat(widget->battery_text, LV_SYMBOL_BATTERY_3);
    } else if (level > 35) {
        strcat(widget->battery_text, LV_SYMBOL_BATTERY_2);
    } else if (level > 5) {
        strcat(widget->battery_text, LV_SYMBOL_BATTERY_1);
    } else {
        strcat(widget->battery_text, LV_SYMBOL_BATTERY_EMPTY);
    }
#endif
    draw_status_bar(widget);
}

static struct output_status_state_bar get_state_bar(const zmk_event_t *_eh) {
    return (struct output_status_state_bar){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open()};
    ;
}

static void set_status_symbol_bar(struct zmk_widget_status_bar *widget,
                                  struct output_status_state_bar state) {
    strcpy(widget->output_text, "");

    switch (state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        strcat(widget->output_text, LV_SYMBOL_USB);
        break;
    case ZMK_TRANSPORT_BLE:
        if (state.active_profile_bonded) {
            if (state.active_profile_connected) {
                snprintf(widget->output_text, sizeof(widget->output_text),
                         LV_SYMBOL_WIFI " %i " LV_SYMBOL_OK,
                         state.selected_endpoint.ble.profile_index + 1);
            } else {
                snprintf(widget->output_text, sizeof(widget->output_text),
                         LV_SYMBOL_WIFI " %i " LV_SYMBOL_CLOSE,
                         state.selected_endpoint.ble.profile_index + 1);
            }
        } else {
            snprintf(widget->output_text, sizeof(widget->output_text),
                     LV_SYMBOL_WIFI " %i " LV_SYMBOL_SETTINGS,
                     state.selected_endpoint.ble.profile_index + 1);
        }
        break;
    }
    draw_status_bar(widget);
}

void battery_status_update_cb_bar(struct battery_status_state_bar state) {
    struct zmk_widget_status_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol_bar(widget, state); }
}

static struct battery_status_state_bar battery_status_get_state_bar(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state_bar) {
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status_bar, struct battery_status_state_bar,
                            battery_status_update_cb_bar, battery_status_get_state_bar)

ZMK_SUBSCRIPTION(widget_battery_status_bar, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status_bar, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static void output_status_update_cb_bar(struct output_status_state_bar state) {
    struct zmk_widget_status_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol_bar(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status_bar, struct output_status_state_bar,
                            output_status_update_cb_bar, get_state_bar)
ZMK_SUBSCRIPTION(widget_output_status_bar, zmk_endpoint_changed);
// We don't get an endpoint changed event when the active profile connects/disconnects
// but there wasn't another endpoint to switch from/to, so update on BLE events too.
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status_bar, zmk_ble_active_profile_changed);
#endif

int zmk_widget_status_bar_init(struct zmk_widget_status_bar *widget, lv_obj_t *parent) {
    sys_slist_append(&widgets, &widget->node);

    widget->canvas = lv_canvas_create(parent);

    lv_draw_label_dsc_init(&widget->battery_label_dsc);
    widget->battery_label_dsc.color = LVGL_FOREGROUND;
    widget->battery_label_dsc.font = &lv_font_montserrat_16;
    widget->battery_label_dsc.align = LV_TEXT_ALIGN_LEFT;
    lv_draw_label_dsc_init(&widget->output_label_dsc);
    widget->output_label_dsc.color = LVGL_FOREGROUND;
    widget->output_label_dsc.font = &lv_font_montserrat_16;
    widget->output_label_dsc.align = LV_TEXT_ALIGN_LEFT;

    widget_battery_status_bar_init();
    widget_output_status_bar_init();
    return 0;
}

lv_obj_t *zmk_widget_status_bar_obj(struct zmk_widget_status_bar *widget) { return widget->canvas; }
