#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#define STATUS_BAR_HEIGHT 40
#define CANVAS_SIZE 68

#define LVGL_BACKGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define LVGL_FOREGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

struct zmk_widget_status_bar {
    sys_snode_t node;
    lv_obj_t *canvas;
    lv_obj_t *draw_dsc;
    lv_color_t cbuf[CANVAS_SIZE * STATUS_BAR_HEIGHT];
    lv_color_t cbuf_tmp[CANVAS_SIZE * STATUS_BAR_HEIGHT];
    char battery_text[9];
    char output_text[10];
    lv_draw_label_dsc_t battery_label_dsc;
    lv_draw_label_dsc_t output_label_dsc;
};

int zmk_widget_status_bar_init(struct zmk_widget_status_bar *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_bar_obj(struct zmk_widget_status_bar *widget);