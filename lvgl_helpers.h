/**
 * @file lvgl_helpers.h
 */

#ifndef LVGL_HELPERS_H
#define LVGL_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#include "driver/spi_common.h"

#include "lvgl_spi_conf.h"
#include "lvgl_tft/disp_driver.h"
#include "lvgl_tft/esp_lcd_backlight.h"
#include "lvgl_touch/touch_driver.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lvgl_i2c_locking(void* leader);

/* Initialize detected SPI and I2C bus and devices */
void lvgl_interface_init(void);

/* Initialize SPI master  */
bool lvgl_spi_driver_init(spi_host_device_t host, int miso_pin, int mosi_pin, int sclk_pin,
    int max_transfer_sz, int dma_channel, int quadwp_pin, int quadhd_pin);

/* Initialize display GPIOs, e.g. DC and RST pins */
void display_bsp_init_io(void);

/* Get display buffer size */
size_t lvgl_get_display_buffer_size(void);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_HELPERS_H */
