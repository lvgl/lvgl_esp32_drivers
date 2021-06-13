#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "epdiy_epaper.h"

#include "epd_driver.h"
#include "epd_highlevel.h"

/**************************************************************************************************
 *      NOTE: This file iis the first version that writes directly on the set_px callback
 *            each pixel into the epaper display buffer. The second version is not epdiy_epaper.cpp
 *            It writes *buf and then it comes as *color_map on the flush callback.
 *            Feel free to experiment with this 2. epdiy_epaper.cpp works better to make a small UX
 **************************************************************************************************/
#define TAG "EPDIY"
EpdiyHighlevelState hl;
uint16_t flushcalls = 0;
uint8_t * framebuffer;
uint8_t temperature = 25;

/* Display initialization routine */
void epdiy_init(void)
{
    epd_init(EPD_OPTIONS_DEFAULT);
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    framebuffer = epd_hl_get_framebuffer(&hl);    
    epd_poweron();
    //Clear all always in init?
    //epd_fullclear(&hl, temperature);
}

uint16_t xo = 0;
uint16_t yo = 0;

/* Required by LVGL */
void epdiy_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    ++flushcalls;
    xo = area->x1;
    yo = area->y1;
    uint16_t w = lv_area_get_width(area);
    uint16_t h = lv_area_get_height(area);

    EpdRect update_area = {
        .x = xo,
        .y = yo,
        .width = w,
        .height = h,
    };

    epd_hl_update_area(&hl, MODE_GC16, temperature, update_area); //update_area

    printf("epdiy_flush %d x:%d y:%d w:%d h:%d\n", flushcalls,xo,yo,w,h);
    /* Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv);
}

/* 
 * Called for each pixel. Designed with the idea to fill the buffer directly, not to set each pixel, see:
 * https://forum.lvgl.io/t/lvgl-port-to-be-used-with-epaper-displays/5630/3
*/
void epdiy_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    // Debug where x y is printed, not all otherwise is too much Serial
    /* if ((int16_t)y%10==0 && flushcalls>0){
        if ((int16_t)x%2==0){
            printf("x%d y%d\n", (int16_t)x, (int16_t)y);
        }
    }
    */

    // Test using RGB232
    int16_t epd_color = 255;
    if ((int16_t)color.full<250) {
        epd_color = (int16_t)color.full/3;
    }

    int16_t x1 = (int16_t)x;
    int16_t y1 = (int16_t)y;

    //Instead of using epd_draw_pixel: Set pixel directly in buffer
    //epd_draw_pixel(x1, y1, epd_color, framebuffer);
    uint8_t *buf_ptr = &framebuffer[y1 * buf_w / 2 + x1 / 2];
    if (x % 2) {
        *buf_ptr = (*buf_ptr & 0x0F) | (epd_color & 0xF0);
    } else {
        *buf_ptr = (*buf_ptr & 0xF0) | (epd_color >> 4);
    }
}
