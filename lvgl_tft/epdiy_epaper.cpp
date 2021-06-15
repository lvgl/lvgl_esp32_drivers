#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epdiy_epaper.h"
#include "epd_driver.h"
#include "epd_highlevel.h"

EpdiyHighlevelState hl;
uint16_t flushcalls = 0;
uint8_t * framebuffer;
uint8_t temperature = 25;
bool init = true;
// MODE_DU: Fast monochrome | MODE_GC16 slow with 16 grayscales
enum EpdDrawMode updateMode = MODE_DU;

/* Display initialization routine */
void epdiy_init(void)
{
    epd_init(EPD_OPTIONS_DEFAULT);
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    framebuffer = epd_hl_get_framebuffer(&hl);    
    epd_poweron();
    //Clear all always in init:
    epd_fullclear(&hl, temperature);
}

/* Suggested by @kisvegabor https://forum.lvgl.io/t/lvgl-port-to-be-used-with-epaper-displays/5630/26 */
void buf_area_to_framebuffer(const lv_area_t *area, const uint8_t *image_data) {
  assert(framebuffer != NULL);
  uint8_t *fb_ptr = &framebuffer[area->y1 * EPD_WIDTH / 2 + area->x1 / 2];
  lv_coord_t img_w = lv_area_get_width(area);
  for(uint32_t y = area->y1; y < area->y2; y++) {
      memcpy(fb_ptr, image_data, img_w / 2);
      fb_ptr += EPD_WIDTH / 2;
      image_data += img_w / 2;
  }
}

/* A copy from epd_copy_to_framebuffer with temporary lenght prediction */
void buf_copy_to_framebuffer(EpdRect image_area, const uint8_t *image_data) {
  assert(framebuffer != NULL);

  for (uint32_t i = 0; i < image_area.width * image_area.height; i++) {
    uint8_t val = (i % 2) ? (image_data[i / 2] & 0xF0) >> 4
                                    : image_data[i / 2] & 0x0F;
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

/* Required by LVGL. Sends the color_map to the screen with a partial update  */
void epdiy_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    ++flushcalls;
    uint16_t w = lv_area_get_width(area);
    uint16_t h = lv_area_get_height(area);

    EpdRect update_area = {
        .x = (uint16_t)area->x1,
        .y = (uint16_t)area->y1,
        .width = w,
        .height = h
    };

    uint8_t* buf = (uint8_t *) color_map;
    // Buffer debug
    /* 
    for (int index=0; index<400; index++) {
        printf("%x ", buf[index]);
    } */

    // UNCOMMENT only one of this options
    // SAFE Option with EPDiy copy of epd_copy_to_framebuffer
    buf_copy_to_framebuffer(update_area, buf);

    //Faster mode suggested in LVGL forum (Leaves ghosting&prints bad sections / experimental) NOTE: Do NOT use in production
    //buf_area_to_framebuffer(area, buf);

    epd_hl_update_area(&hl, updateMode, temperature, update_area); //update_area

    printf("epdiy_flush %d x:%d y:%d w:%d h:%d\n", flushcalls,(uint16_t)area->x1,(uint16_t)area->y1,w,h);
    /* Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv);
}

/* 
 * Called for each pixel. Designed with the idea to fill the buffer directly, not to set each pixel, see LVGL Forum (buf_area_to_framebuffer)
*/
void epdiy_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    // Test using RGB232
    int16_t epd_color = 255;
    if ((int16_t)color.full<250) {
        epd_color = (updateMode==MODE_DU) ? 0 : (int16_t)color.full/3;
    }

    //Instead of using epd_draw_pixel: Set pixel directly in *buf that comes afterwards in flush as *color_map
    uint16_t idx = (int16_t)y * buf_w / 2 + (int16_t)x / 2;
    if (x % 2) {
        buf[idx] = (buf[idx] & 0x0F) | (epd_color & 0xF0);
    } else {
        buf[idx] = (buf[idx] & 0xF0) | (epd_color >> 4);
    }
}
