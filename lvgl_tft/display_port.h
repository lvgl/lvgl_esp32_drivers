#ifndef DISPLAY_PORT_H_
#define DISPLAY_PORT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/**
 * Busy wait delay port
 *
 * @param drv Pointer to driver See @ref lv_disp_drv_t
 * @param delay_ms Delay duration in milliseconds
 */
void display_port_delay(lv_disp_drv_t *drv, uint32_t delay_ms);

/**
 * Backlight control port
 *
 * @param drv Pointer to driver See @ref lv_disp_drv_t
 * @param state State of the backlight signal
 */
void display_port_backlight(lv_disp_drv_t *drv, uint8_t state);

/**
 * DC signal control port
 *
 * @param drv Pointer to driver See @ref lv_disp_drv_t
 * @param state State of the DC signal, 1 for logic high, 0 for logic low
 */
void display_port_gpio_dc(lv_disp_drv_t *drv, uint8_t state);

/**
 * Hardware reset control port
 *
 * @param drv Pointer to driver See @ref lv_disp_drv_t
 * @param state State of the reset signal, 1 for logic high, 0 for logic low
 */
void display_port_gpio_rst(lv_disp_drv_t *drv, uint8_t state);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
