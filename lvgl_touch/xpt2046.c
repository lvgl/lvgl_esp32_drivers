/**
 * @file XPT2046.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "xpt2046.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "tp_spi.h"
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/
#define TAG "XPT2046"

#define CMD_X_READ  0b10010000  // NOTE: XPT2046 data sheet says this is actually Y
#define CMD_Y_READ  0b11010000  // NOTE: XPT2046 data sheet says this is actually X
#define CMD_Z1_READ 0b10110000
#define CMD_Z2_READ 0b11000000
#define Z_MIN 400

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y);
static void xpt2046_avg(int16_t * x, int16_t * y);
static int16_t xpt2046_cmd(uint8_t cmd);

/**********************
 *  STATIC VARIABLES
 **********************/
int16_t avg_buf_x[XPT2046_AVG];
int16_t avg_buf_y[XPT2046_AVG];
uint8_t avg_last;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the XPT2046
 */
void xpt2046_init(void)
{
    gpio_config_t irq_config = {
        .pin_bit_mask = BIT64(XPT2046_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ESP_LOGI(TAG, "XPT2046 Initialization");

    esp_err_t ret = gpio_config(&irq_config);
    assert(ret == ESP_OK);
}

/**
 * Get the current position and state of the touchpad
 * @param data store the read data here
 * @return false: because no more data to be read
 */
bool xpt2046_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    bool valid = false;

    int16_t x = last_x;
    int16_t y = last_y;

    uint8_t irq = gpio_get_level(XPT2046_IRQ);

    if (irq == 0) {
#if XPT2046_TOUCH_CHECK != 0
        int16_t z1 = xpt2046_cmd(CMD_Z1_READ) >> 3;
        int16_t z2 = xpt2046_cmd(CMD_Z2_READ) >> 3;

        // this is not what the confusing datasheet says but it seems to
        // be enough to detect real touches on the panel
        int16_t z = z1 + 4096 - z2;

        // seems the irq can be noisy so we only accept this as a touch if
        // there is some pressure (z) detected
        if (z >= Z_MIN)
        {
#endif
            valid = true;

            x = xpt2046_cmd(CMD_X_READ);
            y = xpt2046_cmd(CMD_Y_READ);
            ESP_LOGI(TAG, "P(%d,%d)", x, y);

            /*Normalize Data back to 12-bits*/
            x = x >> 4;
            y = y >> 4;
            ESP_LOGI(TAG, "P_norm(%d,%d)", x, y);
            
            xpt2046_corr(&x, &y);
            xpt2046_avg(&x, &y);
            last_x = x;
            last_y = y;

            ESP_LOGI(TAG, "x = %d, y = %d", x, y);
#if XPT2046_TOUCH_CHECK != 0
        }
#endif
    }

    if (!valid)
    {
        avg_last = 0;
    }

    data->point.x = x;
    data->point.y = y;
    data->state = valid == false ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;

    return false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int16_t xpt2046_cmd(uint8_t cmd)
{
    uint8_t data[2];
    tp_spi_read_reg(cmd, data, 2);
    int16_t val = (data[0] << 8) | data[1];
    return val;
}

static void xpt2046_corr(int16_t * x, int16_t * y)
{
#if XPT2046_XY_SWAP != 0
	int16_t swap_tmp;
    swap_tmp = *x;
    *x = *y;
    *y = swap_tmp;
#endif

    if((*x) > XPT2046_X_MIN)(*x) -= XPT2046_X_MIN;
    else(*x) = 0;

    if((*y) > XPT2046_Y_MIN)(*y) -= XPT2046_Y_MIN;
    else(*y) = 0;

    (*x) = (uint32_t)((uint32_t)(*x) * LV_HOR_RES) /
           (XPT2046_X_MAX - XPT2046_X_MIN);

    (*y) = (uint32_t)((uint32_t)(*y) * LV_VER_RES) /
           (XPT2046_Y_MAX - XPT2046_Y_MIN);

#if XPT2046_X_INV != 0
    (*x) =  LV_HOR_RES - (*x);
#endif

#if XPT2046_Y_INV != 0
    (*y) =  LV_VER_RES - (*y);
#endif


}


static void xpt2046_avg(int16_t * x, int16_t * y)
{
    /*Shift out the oldest data*/
    uint8_t i;
    for(i = XPT2046_AVG - 1; i > 0 ; i--) {
        avg_buf_x[i] = avg_buf_x[i - 1];
        avg_buf_y[i] = avg_buf_y[i - 1];
    }

    /*Insert the new point*/
    avg_buf_x[0] = *x;
    avg_buf_y[0] = *y;
    if(avg_last < XPT2046_AVG) avg_last++;

    /*Sum the x and y coordinates*/
    int32_t x_sum = 0;
    int32_t y_sum = 0;
    for(i = 0; i < avg_last ; i++) {
        x_sum += avg_buf_x[i];
        y_sum += avg_buf_y[i];
    }

    /*Normalize the sums*/
    (*x) = (int32_t)x_sum / avg_last;
    (*y) = (int32_t)y_sum / avg_last;
}
