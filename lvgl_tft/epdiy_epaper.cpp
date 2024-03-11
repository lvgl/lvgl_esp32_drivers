#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epdiy_epaper.h"
#include "epdiy.h"

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
//    epd_init(&epd_board_v7, &ED097TC2, EPD_LUT_64K);
    epd_init(&epd_board_v7, &EC060KH3, EPD_LUT_64K);
  // Set VCOM for boards that allow to set this in software (in mV).
  // This will print an error if unsupported. In this case,
  // set VCOM using the hardware potentiometer and delete this line.
    epd_set_vcom(1760);
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    framebuffer = epd_hl_get_framebuffer(&hl);    
    epd_poweron();
    //Clear all display in initialization to remove any ghosts
    epd_fullclear(&hl, temperature);
}

/* Suggested by @kisvegabor https://forum.lvgl.io/t/lvgl-port-to-be-used-with-epaper-displays/5630/26 
 * @deprecated 
*/
void buf_area_to_framebuffer(const lv_area_t *area, const uint8_t *image_data) {
  assert(framebuffer != NULL);
  uint8_t *fb_ptr = &framebuffer[area->y1 * epd_width() / 2 + area->x1 / 2];
  lv_coord_t img_w = lv_area_get_width(area);
  for(uint32_t y = area->y1; y < area->y2; y++) {
      memcpy(fb_ptr, image_data, img_w / 2);
      fb_ptr += epd_width() / 2;
      image_data += img_w / 2;
  }
}

/* A copy from epd_copy_to_framebuffer with temporary lenght prediction */
void buf_copy_to_framebuffer(EpdRect image_area, const uint8_t *image_data) {
  assert(framebuffer != NULL);
  int x, xx = image_area.x;
  int y, yy = image_area.y;
  int w = image_area.width;
  int h = image_area.height;
  const uint8_t ucCLRMask[3] = {0xc0,0x38,0x7};
  uint8_t uc, *s, *d;
// source data is one byte per pixel (RGB233)
   for (y=yy; y<(yy + h); y++) {
       int i = (xx+y) % 3; // which color filter is in use?
       s = (uint8_t *)&image_data[(y-yy) * w];
       d = &framebuffer[(y * epd_width() / 2) + (xx / 2)];
       x = xx;
       if (x & 1) {
          uc = d[0] & 0xf0; // special case for odd starting pixel
          if (s[0] & ucCLRMask[i])
              uc |= 0xf;
          i++;
          s++;
          *d++ = uc;
          x++;
          if (i >= 3) i-=3;
       }
       for (; x<(xx + w); x+=2) { // work 2 pixels at a time
            uc = 0;
            if (s[0] & ucCLRMask[i]) uc |= 0xf;
            i++; if (i >= 3) i -= 3;
            if (s[1] & ucCLRMask[i]) uc |= 0xf0;
            i++; if (i >= 3) i -= 3;
            *d++ = uc;
            s += 2;
       } // for x
   } // for y
} /* buf_copy_to_framebuffer() */

/* Required by LVGL. Sends the color_map to the screen with a partial update  */
void epdiy_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
static int x1=65535,y1=65535,x2=-1,y2=-1;

    ++flushcalls;
    uint16_t w = lv_area_get_width(area);
    uint16_t h = lv_area_get_height(area);

    EpdRect update_area = {
        .x = (uint16_t)area->x1,
        .y = (uint16_t)area->y1,
        .width = w,
        .height = h
    };

    // capture the upper left and lower right corners
    if (area->x1 < x1) x1 = area->x1;
    if (area->y1 < y1) y1 = area->y1;
    if (area->x2 > x2) x2 = area->x2;
    if (area->y2 > y2) y2 = area->y2;

    uint8_t* buf = (uint8_t *) color_map;
    // Buffer debug
    /* for (int index=0; index<400; index++) {
        printf("%x ", buf[index]);
    } */
    // This is the slower version that works good without leaving any white line
    buf_copy_to_framebuffer(update_area, buf);

    //Faster mode suggested in LVGL forum (Leaves ghosting&prints bad sections / experimental) NOTE: Do NOT use in production
    //buf_area_to_framebuffer(area, buf);
    if (lv_disp_flush_is_last(drv)) { // only send to e-paper when complete
        update_area.x = x1;
        update_area.y = y1;
        update_area.width = (x2-x1)+1;
        update_area.height = (y2-y1)+1;
        epd_hl_update_area(&hl, updateMode, temperature, update_area); //update_area
        x1 = y1 = 65535; x2 = y2 = -1; // reset update boundary
    }
    //printf("epdiy_flush %d x:%d y:%d w:%d h:%d\n", flushcalls,(uint16_t)area->x1,(uint16_t)area->y1,w,h);
    /* Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv); // do this after the check for "is_last"
}

/* 
 * Called for each pixel. Designed with the idea to fill the buffer directly, not to set each pixel, see LVGL Forum (buf_area_to_framebuffer)
*/
void epdiy_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    buf[(y * buf_w)+x] = color.full;
}
