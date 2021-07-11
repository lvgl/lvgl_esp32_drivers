/**
 * @file esp_lcd_backlight.h
 */

#ifndef ESP_LCD_BACKLIGHT_H
#define ESP_LCD_BACKLIGHT_H

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


/*********************
 *      DEFINES
 *********************/
#if CONFIG_LV_ENABLE_BACKLIGHT_CONTROL
#define DISP_PIN_BCKL   CONFIG_LV_DISP_PIN_BCKL
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void disp_brightness_control_enable(void);
void disp_set_brightness(uint16_t brightness);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ESP_LCD_BACKLIGHT_H*/