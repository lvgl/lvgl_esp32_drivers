#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "sharp_mip.h" 
#include <Adafruit_SharpMem.h>

/** test Display dimensions
 * Do not forget to set: menuconfig -> Components -> LVGL configuration
 * Max. Horizontal resolution 400 -> WIDTH of your LCD
 * Max. Vertical resolution   240 -> HEIGHT
 */

/*
 * Return the draw_buf byte index corresponding to the pixel
 * relatives coordinates (x, y) in the area.
 * The area is rounded to a whole screen line.
 */
#define BUFIDX(x, y)  (((x) >> 3) + ((y) * (2 + (LV_HOR_RES_MAX >> 3))) + 2)

// SHARP LCD Class
// PCB Pins for LCD SPI
#define SHARP_SCK  6
#define SHARP_MOSI 0
#define SHARP_SS   7

// External COM Inversion Signal
#define LCD_EXTMODE GPIO_NUM_3
#define LCD_DISP    GPIO_NUM_2
// Set the size of the display here, e.g. 128x128
Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, LV_HOR_RES_MAX, LV_VER_RES_MAX);

/*********************
 *      DEFINES
 *********************/
#define TAG "SHARP"

uint16_t flushcalls = 0;

void delay(uint32_t period_ms) {
    vTaskDelay(pdMS_TO_TICKS(period_ms));
}

/* Display initialization routine */
void sharp_init(void)
{
    printf("SHAROP LCD init\n");
    // Display pins config
    gpio_set_direction(LCD_EXTMODE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_DISP, GPIO_MODE_OUTPUT);// Display On(High)/Off(Low) 
    gpio_set_level(LCD_DISP, 1);
    gpio_set_level(LCD_EXTMODE, 1);
    display.begin();
}

/**
 * @brief rounds area before writing
 * 
 * @param disp_drv 
 * @param area 
 * @deprecated
 */
void sharp_mip_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area) {
  /* Round area to the whole LINE */
    area->x1 = 0;
    area->x2 = LV_HOR_RES_MAX - 1;
}

/* Required by LVGL */
void sharp_flush(lv_disp_drv_t *drv, lv_area_t *area, lv_color_t *color_map)
{
    /* Round area to the whole LINE */
    /* area->x1 = 0;
    area->x2 = LV_HOR_RES_MAX - 1; */
    //lv_refr_set_round_cb(sharp_mip_rounder); // NOT applicable to this version of LVGL
    // FORCE Flushing Whole display (Really unnecesary, just a demo, since I don't know how to refresh an *LCD partial only*)

    ++flushcalls;
    // x2:%d  y2:%d 
    printf("flush %d x1:%d y1:%d x2:%d y2:%d  w:%d h:%d\n",
    flushcalls,area->x1,area->y1,area->x2,area->y2,lv_area_get_width(area),lv_area_get_height(area));

    // FULL Refresh does not work nice unless you force it on newer versions of LVGL (see lv_refr_set_round_cb)
    //display.refresh();
    display.refreshLines(area->x1, area->x1, area->y1,area->y2);
    /* IMPORTANT:
     * Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv);
}

/* Called for each pixel */
void sharp_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    // Test using RGB232: Otherwise you can directly use color.full if MONOCHROME
    int16_t lcd_color = 1; // WHITE

    // Color setting use: RGB232
    // Only monochrome:   All what is not white, turn black
    if (color.full<254) { 
        lcd_color = 0;        
    }
    display.drawPixel((int16_t)x, (int16_t)y, lcd_color);

    //If not drawing anything: Debug to see if this function is called:
    //printf("set_px %d %d R:%d G:%d B:%d\n",(int16_t)x,(int16_t)y, color.ch.red, color.ch.green, color.ch.blue);
    
}
