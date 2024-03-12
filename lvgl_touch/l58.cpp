/*
* Copyright © 2020 Martin Fasani
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

#include <esp_log.h>
#include <driver/i2c.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#else
#include <lvgl/lvgl.h>
#endif
#include "l58.h"
#include "../lvgl_i2c_conf.h"
// Cale touch implementation
#include "L58Touch.h"
L58Touch Touch(CONFIG_LV_TOUCH_INT);
#define TAG "L58"

TPoint point;

/**
  * @brief  Initialize for FT6x36 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of FT6X36).
  * @retval None
  */
void l58_init() {
  ESP_LOGI(TAG, "l58_init() Touch initialized");
  Touch.begin(960, 540);
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
bool l58_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    point = Touch.loop();
    data->point.x = point.x;
    data->point.y = point.y;
    if (point.event == 3) {
      data->state = LV_INDEV_STATE_PR;
    } else {
      data->state = LV_INDEV_STATE_REL;
    }
    return false;
}
