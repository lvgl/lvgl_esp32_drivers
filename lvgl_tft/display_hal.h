#ifndef DISPLAY_HAL_H_
#define DISPLAY_HAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/* NOTE We could remove this function from here, because it's used to
 * initialize the GPIOS on the MCU */
void display_hal_init_io(void);

/* NOTE drv parameter is meant to be used in multi display projects, so the
 * user could distinguish multiple displays on their hal implementation */
void display_hal_delay(lv_disp_drv_t *drv, uint32_t delay_ms);
void display_hal_backlight(lv_disp_drv_t *drv, uint8_t state);
void display_hal_gpio_dc(lv_disp_drv_t *drv, uint8_t state);
void display_hal_gpio_rst(lv_disp_drv_t *drv, uint8_t state);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
