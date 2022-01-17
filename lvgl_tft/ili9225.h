/**
 * @file lv_templ.h
 *
 */

#ifndef ILI9225_H
#define ILI9225_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "sdkconfig.h"

/*********************
 *      DEFINES
 *********************/
#define ILI9225_DC        CONFIG_LV_DISP_PIN_DC
#define ILI9225_USE_RST   CONFIG_LV_DISP_USE_RST
#define ILI9225_RST       CONFIG_LV_DISP_PIN_RST
#define ILI9225_INVERT_COLORS CONFIG_LV_INVERT_COLORS

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void ili9225_init(void);
void ili9225_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ILI9225_H*/
