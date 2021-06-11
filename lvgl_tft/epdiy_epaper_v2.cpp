#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "epdiy_epaper.h"

#include "epd_driver.h"
#include "epd_highlevel.h"
// Usable size
#include <malloc.h>
/*********************
 *      DEFINES
 *********************/
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
    epd_fullclear(&hl, temperature);
}

uint16_t xo = 0;
uint16_t yo = 0;
uint16_t rx = 0;
uint16_t ry = 0;
uint16_t rw = 0;
uint16_t rh = 0;

void buf_copy_to_framebuffer(EpdRect image_area, const uint8_t *image_data) {

  assert(framebuffer != NULL);

  for (uint32_t i = 0; i < image_area.width * image_area.height; i++) {

    uint32_t value_index = i;
    // for images of uneven width consume an additional nibble per row.
    if (image_area.width % 2) {
      value_index += i / image_area.width;
    }
    // Get out if we get the end of the buffer  (image_data[value_index / 2]== '\0')  111684
    if (value_index / 2 > 111600) {
        printf("FOUND END incr:%d img[idx]:%d Aw:%d Ah:%d\n", i, 
        value_index / 2,image_area.width,image_area.height);
        break;
    } 
    
    uint8_t val = (value_index % 2) ? (image_data[value_index / 2] & 0xF0) >> 4
                                    : image_data[value_index / 2] & 0x0F;

    int xx = image_area.x + i % image_area.width;
    if (xx < 0 || xx >= EPD_WIDTH) {
      continue;
    }
    int yy = image_area.y + i / image_area.width;
    if (yy < 0 || yy >= EPD_HEIGHT) {
      continue;
    }
    uint8_t *buf_ptr = &framebuffer[yy * EPD_WIDTH / 2 + xx / 2];
    if (xx % 2) {
      *buf_ptr = (*buf_ptr & 0x0F) | (val << 4);
    } else {
      *buf_ptr = (*buf_ptr & 0xF0) | val;
    }
  }
}

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

    uint8_t* buf = (uint8_t *) color_map;
    // Buffer debug
    /* 
    for (int index=0; index<400; index++) {
        printf("%x ", buf[index]);
    } */

    // Does not work for full screen update yet (Should check large of bug)
    if (flushcalls>0){
      buf_copy_to_framebuffer(update_area, buf);
    }

    epd_hl_update_area(&hl, MODE_GC16, temperature, update_area); //update_area

    printf("epdiy_flush %d x:%d y:%d w:%d h:%d\n", flushcalls,xo,yo,w,h);
    //printf("epdiy_flush Rounder() x:%d y:%d w:%d h:%d\n",rx,ry,rw,rh);
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
    // Test using RGB232
    int16_t epd_color = 255;
    if ((int16_t)color.full<250) {
        epd_color = (int16_t)color.full/3;
    }

    //Instead of using epd_draw_pixel: Set pixel directly in buffer
    uint16_t idx = (int16_t)y * buf_w / 2 + (int16_t)x / 2;
    if (x % 2) {
        buf[idx] = (buf[idx] & 0x0F) | (epd_color & 0xF0);
    } else {
        buf[idx] = (buf[idx] & 0xF0) | (epd_color >> 4);
    }

    // Directly in framebuffer for epaper
    /* uint8_t *buf_ptr = &framebuffer[(int16_t)y * buf_w / 2 + (int16_t)x / 2];
    if (x % 2) {
        *buf_ptr = (*buf_ptr & 0x0F) | (epd_color & 0xF0);
    } else {
        *buf_ptr = (*buf_ptr & 0xF0) | (epd_color >> 4);
    } */
}

void epdiy_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area) {
    // Print coordinates to understand what rounder is
    /* rx = (int16_t)area->x1;
    ry = (int16_t)area->y1;
    rw = (int16_t)lv_area_get_width(area);
    rh = (int16_t)lv_area_get_height(area);
    printf("R x:%d y:%d y1:%d w:%d h:%d\n",rx,ry,area->y1,rw,rh); */

    // Force y to be 0: Make some things better, screws other things
    //area->y1 = 0;
}