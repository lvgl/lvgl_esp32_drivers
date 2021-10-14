/**
 * @file lv_templ.h
 *
 */

#ifndef ILI9341_H
#define ILI9341_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "display_config.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void ili9341_init(lv_disp_drv_t * drv);
void ili9341_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);
void ili9341_sleep_in(lv_disp_drv_t * drv);
void ili9341_sleep_out(lv_disp_drv_t *drv);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ILI9341_H*/
