/**
 * Display class for generic e-Paper driven by EPDiy class
*/
#ifndef CALEPD_H
#define CALEPD_H

#define EPDIY_COLUMNS          (LV_HOR_RES_MAX / 8)

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
 #include "lvgl/lvgl.h"
#endif
#include "sdkconfig.h"

/* Configure your display */
void calepd_init(void);

/* LVGL callbacks */
void calepd_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/* Only for monochrome displays. But we use epdiy_set_px also for epapers */
//void epdiy_rounder(lv_disp_drv_t *disp_drv, lv_area_t *area);
void calepd_set_px_cb(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* EPDIY_H */