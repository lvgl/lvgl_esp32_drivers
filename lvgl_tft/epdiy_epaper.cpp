#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "epdiy_epaper.h"

// NOTE: This needs Epdiy component https://github.com/vroland/epdiy
// Run idf.py menuconfig-> Component Config -> E-Paper driver and select:
// Display type: LILIGO 4.7 ED047TC1
// Board: LILIGO T5-4.7 Epaper
// In the same section Component Config -> ESP32 Specifics -> Enable PSRAM
#include "parallel/ED047TC1.h"
Ed047TC1 display;

/*********************
 *      DEFINES
 *********************/
#define TAG "EPDIY"

uint16_t flushcalls = 0;

/* Display initialization routine */
void epdiy_init(void)
{
    printf("epdiy_init\n");
    display.init();
    display.setRotation(0);
    display.clearScreen();
}

/* Required by LVGL */
void epdiy_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    ++flushcalls;
    printf("epdiy_flush %d\n", flushcalls);

    display.update();
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv);
}

/* Called for each pixel */
void epdiy_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    // Is printing Y axis only till 40 px: And flushing too many times
    //printf("%d ", (int16_t)y); // Works and prints "Hello world"
    //printf("%d ",(int16_t)color.full); // Debug colors
    // Test using RGB232
    int16_t epd_color = EPD_WHITE;

    // Color setting use: RGB232
    if ((int16_t)color.full<250) {
        epd_color = (int16_t)color.full/3;
    }
    display.drawPixel((int16_t)x, (int16_t)y, epd_color);
}
