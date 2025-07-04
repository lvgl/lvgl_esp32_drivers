/*
* Copyright © 2021 Sturnus Inc.

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
#include "gt911.h"

#include "lvgl_i2c/i2c_manager.h"

#define TAG "GT911"

#define CTP_PIN_RESET GPIO_NUM_32
#define CTP_PIN_INT GPIO_NUM_5
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(50))) {
            //printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

gt911_status_t gt911_status;
bool isinverted_gt911 = true;
//TODO: handle multibyte read and refactor to just one read transaction
esp_err_t gt911_i2c_read(uint8_t slave_addr, uint16_t register_addr, uint8_t *data_buf, uint8_t len) {
    return lvgl_i2c_read(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, data_buf, len);
}

esp_err_t gt911_i2c_write8(uint8_t slave_addr, uint16_t register_addr, uint8_t data) {
    uint8_t buffer = data;
    return lvgl_i2c_write(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, &buffer, 1);
}

/**
  * @brief  Initialize for GT911 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of GT911).
  * @retval None
  */
void gt911_init(uint8_t dev_addr) {
    if (!gt911_status.inited) {
        gt911_status.i2c_dev_addr = dev_addr;
        uint8_t data_buf;
        esp_err_t ret;

        gpio_set_drive_capability(CTP_PIN_RESET, GPIO_DRIVE_CAP_3);
        gpio_set_drive_capability(CTP_PIN_INT, GPIO_DRIVE_CAP_3);

        // //create a queue to handle gpio event from isr
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        //start gpio task
        xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

        //CTP Reset, low level enable
        // gpio_config_t io_conf = {
        //     .pin_bit_mask = (1ULL << CTP_PIN_RESET),
        //     .mode = GPIO_MODE_OUTPUT,
        //     .intr_type = GPIO_INTR_DISABLE,
        //     .pull_down_en = GPIO_PULLDOWN_DISABLE,
        //     .pull_up_en = GPIO_PULLUP_ENABLE,
        // };
        //CTP INT, 
        gpio_config_t io_conf_int = {
            .pin_bit_mask = (1ULL << CTP_PIN_INT) ,
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .intr_type = GPIO_INTR_LOW_LEVEL,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLDOWN_DISABLE,
        };
        // // ESP_ERROR_CHECK(gpio_config(&io_conf));
        // ESP_ERROR_CHECK(gpio_config(&io_conf_int));

        

        ESP_ERROR_CHECK(gpio_set_direction(CTP_PIN_RESET, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_pull_mode(CTP_PIN_RESET, GPIO_PULLUP_ONLY));
        ESP_ERROR_CHECK(gpio_set_intr_type(CTP_PIN_RESET,GPIO_INTR_DISABLE));
        ESP_ERROR_CHECK(gpio_set_level(CTP_PIN_RESET, 0));
        ESP_ERROR_CHECK(gpio_set_level(CTP_PIN_RESET, 1));
        ESP_LOGI(TAG, "GPIO RESET CTP Enabled");
        
        
        
        ESP_ERROR_CHECK(gpio_set_direction(CTP_PIN_INT, GPIO_MODE_INPUT));
        // ESP_ERROR_CHECK(gpio_set_pull_mode(CTP_PIN_INT, GPIO_PULLDOWN_ONLY)); // no
        // ESP_ERROR_CHECK(gpio_set_intr_type(CTP_PIN_INT,GPIO_INTR_DISABLE)); // no
        // ESP_ERROR_CHECK(gpio_set_level(CTP_PIN_INT, 0));
        ESP_ERROR_CHECK(gpio_set_pull_mode(CTP_PIN_INT, GPIO_PULLUP_ONLY));


        // ESP_ERROR_CHECK(gpio_set_pull_mode(CTP_PIN_INT, GPIO_FLOATING)); //no

        ESP_ERROR_CHECK(gpio_set_intr_type(CTP_PIN_INT,GPIO_INTR_NEGEDGE));
        ESP_ERROR_CHECK(gpio_intr_enable(CTP_PIN_INT));
       
        // ESP_ERROR_CHECK(gpio_intr_disable(CTP_PIN_INT));  //no

        // if HAL_GPIO used, no longer need to install isr service.
        //ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));

        gpio_isr_handler_add(CTP_PIN_INT, gpio_isr_handler, (void*) CTP_PIN_INT); //callback...
        ESP_LOGI(TAG, "GPIO INT CTP  Enabled");
        // vTaskDelay(500 / portTICK_PERIOD_MS);


        //  gpio_set_level(CTP_PIN_RESET, 0);  //Force high to reset CTP

        ESP_LOGI(TAG, "Checking for GT911 Touch Controller");
        
        if ((ret = gt911_i2c_read(dev_addr, GT911_PRODUCT_ID1, &data_buf, 1) != ESP_OK)) {
            // ESP_LOGE(TAG, "Error reading from device: %s",
            //             esp_err_to_name(ret));    // Only show error the first time
            ESP_LOGI(TAG, "Error reading from device: %s",esp_err_to_name(ret));
            // gt911_reset_CTP();
            return;
        }

        // Read 4 bytes for Product ID in ASCII
        for (int i = 0; i < GT911_PRODUCT_ID_LEN; i++) {
            gt911_i2c_read(dev_addr, (GT911_PRODUCT_ID1 + i), (uint8_t *)&(gt911_status.product_id[i]), 1);
        }
        ESP_LOGI(TAG, "\tProduct ID: %s", gt911_status.product_id);

        gt911_i2c_read(dev_addr, GT911_VENDOR_ID, &data_buf, 1);
        ESP_LOGI(TAG, "\tVendor ID: 0x%02x", data_buf);

        gt911_i2c_read(dev_addr, GT911_X_COORD_RES_L, &data_buf, 1);
        gt911_status.max_x_coord = data_buf;
        gt911_i2c_read(dev_addr, GT911_X_COORD_RES_H, &data_buf, 1);
        gt911_status.max_x_coord |= ((uint16_t)data_buf << 8);
        ESP_LOGI(TAG, "\tX Resolution: %d", gt911_status.max_x_coord);

        gt911_i2c_read(dev_addr, GT911_Y_COORD_RES_L, &data_buf, 1);
        gt911_status.max_y_coord = data_buf;
        gt911_i2c_read(dev_addr, GT911_Y_COORD_RES_H, &data_buf, 1);
        gt911_status.max_y_coord |= ((uint16_t)data_buf << 8);
        ESP_LOGI(TAG, "\tY Resolution: %d", gt911_status.max_y_coord);
        gt911_status.inited = true;

        //I2C communication is valid, but the calibration is incorrect, restart the display
        if(gt911_status.inited == true  ){
            if(gt911_status.max_x_coord == 0){
                ESP_LOGI(TAG, "\tX Resolution: %d IS incorrect, restarting ESP32", gt911_status.max_y_coord);
                esp_restart();
            }else if(gt911_status.max_x_coord >= 320){
                ESP_LOGI(TAG, "\tX Resolution: %d IS incorrect, restarting ESP32", gt911_status.max_y_coord);
                esp_restart();
            }
        }

    }
}

//"Low level Enable" 
void gt911_reset_CTP(){
    gpio_set_level(CTP_PIN_RESET, 0);  //Force high to reset CTP
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(CTP_PIN_RESET, 1);  //Force high to reset CTP
    ESP_LOGI(TAG, "GT911 RESET Completed...restarting display");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    //Will need to restart display..
    // esp_restart();
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
bool gt911_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint8_t touch_pnt_cnt;        // Number of detected touch points
    static int16_t last_x = 0;  // 12bit pixel value
    static int16_t last_y = 0;  // 12bit pixel value
    uint8_t data_buf;
    uint8_t status_reg;

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_STATUS_REG, &status_reg, 1);
//    ESP_LOGI(TAG, "\tstatus: 0x%02x", status_reg);
    touch_pnt_cnt = status_reg & 0x0F;
    if ((status_reg & 0x80) || (touch_pnt_cnt < 6)) {
        //Reset Status Reg Value
        gt911_i2c_write8(gt911_status.i2c_dev_addr, GT911_STATUS_REG, 0x00);
    }
    if (touch_pnt_cnt != 1) {    // ignore no touch & multi touch
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

//    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_TRACK_ID1, &data_buf, 1);
//    ESP_LOGI(TAG, "\ttrack_id: %d", data_buf);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_L, &data_buf, 1);
    last_x = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_H, &data_buf, 1);
    last_x |= ((uint16_t)data_buf << 8);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_L, &data_buf, 1);
    last_y = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_H, &data_buf, 1);
    last_y |= ((uint16_t)data_buf << 8);
    if( isinverted_gt911 == false){
        last_x = gt911_status.max_x_coord - last_x;
        last_y = gt911_status.max_y_coord - last_y;
    }

// #if CONFIG_LV_GT911_INVERT_X
//     last_x = gt911_status.max_x_coord - last_x;
// #endif
// #if CONFIG_LV_GT911_INVERT_Y
//     last_y = gt911_status.max_y_coord - last_y;
// #endif
#if CONFIG_LV_GT911_SWAPXY
    int16_t swap_buf = last_x;
    last_x = last_y;
    last_y = swap_buf;
#endif
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;
    ESP_LOGI(TAG, "X=%u Y=%u", data->point.x, data->point.y);
    return false;
}
