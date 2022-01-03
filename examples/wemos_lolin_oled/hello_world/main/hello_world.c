/* Hello world on Wemos Lolin32 board
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 1

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lvgl_tick_inc(void *arg);
static void guiTask(void *pvParameter);

/**********************
 *   APPLICATION MAIN
 **********************/
void app_main()
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
}

static void guiTask(void *pvParameter)
{
    (void) pvParameter;
    lv_init();
    
    /* Initialize the needed peripherals */
    lvgl_interface_init();
    /* Initialize needed GPIOs, e.g. backlight, reset GPIOs */
    display_bsp_init_io();
    /* ToDo Initialize used display driver passing registered lv_disp_drv_t as parameter */

    size_t display_buffer_size = lvgl_get_display_buffer_size();
    lv_color_t* buf1 = heap_caps_malloc(display_buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, display_buffer_size * 8);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
    disp_drv.draw_buf = &disp_buf;
    /* LVGL v8: Set display horizontal and vertical resolution (in pixels), it's no longer done with lv_conf.h */
    disp_drv.hor_res = 128u;
    disp_drv.ver_res = 64u;
    lv_disp_drv_register(&disp_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lvgl_tick_inc,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create a Hellow World label on the currently active screen */
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    /* LVGL v8 lv_label_create no longer takes 2 parameters */
    lv_obj_t *label1 =  lv_label_create(scr);
    lv_label_set_text(label1, "Hello\nworld");

    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
    }
    free(buf1);
}

static void lvgl_tick_inc(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
