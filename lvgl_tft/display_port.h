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

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CMD_WIDTH_8BITS,
    CMD_WIDTH_16BITS,
    CMD_WIDTH_INVALID,
} cmd_width_t;

typedef enum {
    DATA_XFER_MODE_SYNC,
    DATA_XFER_MODE_ASYNC,
} data_xfer_mode_t;

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

/**
 * Display is busy port
 *
 * @param drv Pointer to driver See @ref lv_disp_drv_t
 *
 * @retval Returns false when display is not busy, true otherwise.
 */
bool display_port_gpio_is_busy(lv_disp_drv_t *drv);

/**
 * Send cmd to display
 * 
 * @param drv Pointer to driver
 * @param cmd Command to send
 * @param cmd_width Width of the command (in bits) to be sent, see @ref cmd_width_t
 * @param args Pointer to arguments, use NULL to send command without arguments
 * @param args_len Arguments length (in bytes) to be sent
 */
void display_interface_send_cmd(lv_disp_drv_t *drv, uint32_t cmd, cmd_width_t cmd_width, void *args, size_t args_len);

/**
 * Send (image) data to display
 *
 * @param drv Pointer to driver
 * @param data Pointer to data to be sent
 * @param len Data length (in bytes) to be sent
 * @param mode Data transfer mode, sync (polling) and async, lv_disp_flush must
 *             be called when finishing the data transfer
 */
void display_interface_send_data(lv_disp_drv_t *drv, void *data, size_t len, data_xfer_mode_t mode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
