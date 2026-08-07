/*
* Copyright © 2023 Fasani Corp.

* Permission is hereby granted, free of charge, to any person obtaining a copy of this 
* software and associated documentation files (the “Software”), to deal in the Software 
* without restriction, including without limitation the rights to use, copy, modify, merge, 
* publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
* to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or 
* substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
* PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
* FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
* SOFTWARE.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_touch_gt911.h"
#include <esp_log.h>
#include "driver/i2c.h"

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#else
#include <lvgl/lvgl.h>
#endif
#include "gt911.h"

#define TAG "GT911"

#ifdef CONFIG_LV_TOUCH_I2C_PORT_0
    #define I2C_PORT I2C_NUM_0
#endif
#ifdef CONFIG_LV_TOUCH_I2C_PORT_1
    #define I2C_PORT I2C_NUM_1
#endif
// When the touch panel has different pixels definition
float x_adjust = 1.55;
float y_adjust = 0.8;
esp_lcd_touch_handle_t tp;

/**
  * @brief  Initialize for GT911 communication via I2C
  * @retval None
  */
void gt911_init(uint8_t dev_addr) {
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 1025,
        .y_max = 770,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    
    esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_PORT, &tp_io_config, &tp_io_handle);

    esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
bool gt911_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    esp_lcd_touch_read_data(tp);

    uint16_t touch_x[2];
    uint16_t touch_y[2];
    uint16_t touch_strength[2];
    uint8_t touch_cnt = 0;
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 2);
    if (touchpad_pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = (int)(touch_x[0]*x_adjust);
        data->point.y = (int)(touch_y[0]*y_adjust);
        ESP_LOGI(TAG, "X=%d Y=%d", (int)data->point.x, (int)data->point.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
        data->point.x = -1;
        data->point.y = -1;
    }
    return false;
}
