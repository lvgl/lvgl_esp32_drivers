#include "ssd1351.h"
#include "assert.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "../ssd1306/src/ssd1306.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ssd1351"

/* Example of function to show some text in display */
static void textDemo()
{
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1306_clearScreen8();
    ssd1306_setColor(RGB_COLOR8(255,255,0));
    ssd1306_printFixed8(0,  8, "Normal text", STYLE_NORMAL);
    ssd1306_setColor(RGB_COLOR8(0,255,0));
    ssd1306_printFixed8(0, 16, "bold text?", STYLE_BOLD);
    ssd1306_setColor(RGB_COLOR8(0,255,255));
    ssd1306_printFixed8(0, 24, "Italic text?", STYLE_ITALIC);
    ssd1306_negativeMode();
    ssd1306_setColor(RGB_COLOR8(255,255,255));
    ssd1306_printFixed8(0, 32, "Inverted bold?", STYLE_BOLD);
    ssd1306_positiveMode();
}
/*The LCD needs a bunch of command/argument values to be initialized. They are
 * stored in this struct. */
void ssd1351_init(void) {
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1351_128x128_spi_init(SSD1351_RST, 17, SSD1351_DC);
    ssd1306_setMode( LCD_MODE_NORMAL );
    ssd1306_clearScreen8( );
    ESP_LOGI(TAG, "Trying to fill.");
    /*Create a display buffer*/
    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * 10];                      /*A buffer for 10 rows*/
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);   /*Initialize the display buffer*/

    /*Create a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = ssd1351_flush;
    disp_drv.hor_res = 128;
    disp_drv.ver_res = 128;
    disp_drv.antialiasing = 1;

    /*Finally, register the driver*/
    lv_disp_drv_register(&disp_drv);
}

void ssd1351_flush(lv_disp_drv_t *drv, const lv_area_t *area,
                   lv_color_t *color_map) {
    ESP_LOGI(TAG, "Flushing");
    int32_t x, y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            ssd1306_putColorPixel8(x, y, !lv_color_to1(*color_map));
            printf("X=%d Y=%d", x, y);
            color_map++;
        }
    }

    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(drv);
}
