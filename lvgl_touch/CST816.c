/*
* Copyright © 2022 Band Industries Inc.

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
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#else
#include <lvgl/lvgl.h>
#endif
#include "CST816.h"

#include "lvgl_i2c/i2c_manager.h"
#include "band_es8388.h"

#define TAG "CST816"

cst816_status_t cst816_status;

//[BJ] TODO: Below functions are ADF specific, if I am to contribute to main repo this needs to be resolved!
static esp_err_t _cst816_i2c_read(uint8_t slave_addr, uint8_t register_addr, uint8_t *data_buf, uint8_t len) {
    return band_i2c_read(slave_addr, register_addr, data_buf, len);
    //return lvgl_i2c_read(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, data_buf, len);
}

static esp_err_t _cst816_i2c_write8(uint8_t slave_addr, uint8_t register_addr, uint8_t data) {
    //uint8_t buffer = data;
    return band_i2c_write(slave_addr, register_addr, &data, 1);
    //return lvgl_i2c_write(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, &buffer, 1);
}

/**
  * @brief  Initialize for cst816 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of cst816).
  * @retval None
  */
void cst816_init(uint8_t dev_addr) {
     if (!cst816_status.inited) 
     {
        cst816_status.i2c_dev_addr = dev_addr;
        uint8_t version;
        uint8_t versionInfo[3];
        esp_err_t ret;
        ESP_LOGI(TAG, "Initializing cst816 %d", dev_addr);
        if ((ret = _cst816_i2c_read(dev_addr, 0x15, &version, 1) != ESP_OK))
        {
            ESP_LOGE(TAG, "Error reading version from device: %s",
            esp_err_to_name(ret));    // Only show error the first time
            return;
        }
        if ((ret = _cst816_i2c_read(dev_addr, 0xA7, versionInfo, 3) != ESP_OK))
        {
            ESP_LOGE(TAG, "Error reading versionInfo from device: %s",
            esp_err_to_name(ret));    // Only show error the first time
            return;
        }
        ESP_LOGI(TAG, "CST816 version %d, versionInfo: %d.%d.%d", version, versionInfo[0], versionInfo[1], versionInfo[2]);
        cst816_status.inited = true;
    }
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
#define SWAPXY 1
#define INVERTX 1
#define INVERTY 0



bool cst816_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {

    uint8_t data_raw[8];
    _cst816_i2c_read(cst816_status.i2c_dev_addr, 0x01, data_raw, 6);
    


    uint8_t gestureID = data_raw[0];
    uint8_t points = data_raw[1];
    uint8_t event = data_raw[2] >> 6;
    uint16_t point_x = 0;
    uint16_t point_y = 0;
    if(points == 0)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return false;

    }
    
    data->state = LV_INDEV_STATE_PRESSED;
    point_x = (data_raw[3] | ((data_raw[2] & 0x0F) << 8));
    point_y = (data_raw[5] | ((data_raw[4] & 0x0F) << 8));

#if CONFIG_LV_SWAPXY_CST816
    int temp;
    temp = point_y;
    point_y = point_x;
    point_x = temp;
#endif

#if CONFIG_LV_INVERT_X_CST816  
    point_x = CONFIG_LV_TOUCH_X_MAX_CST816 - point_x;
#endif

#if CONFIG_LV_INVERT_Y_CST816
    point_y = CONFIG_LV_TOUCH_Y_MAX_CST816 - point_y;
#endif

    data->point.x = point_x;
    data->point.y = point_y;
    ESP_LOGI(TAG, "gestureID %d, points %d, event %d X=%u Y=%u", gestureID, points, event, data->point.x, data->point.y);
    return false;
}
