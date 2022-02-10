#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_port.h"

#include "sdkconfig.h"
#include "driver/gpio.h"

#include "disp_spi.h"
#include "lvgl_i2c/i2c_manager.h"

#include <stdbool.h>
#include <assert.h>

/* NOTE: Needed by the I2C Manager */
#define OLED_I2C_PORT                       (CONFIG_LV_I2C_DISPLAY_PORT)
/* FIXME: Be able to get display driver I2C address from Kconfig */
#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306)
#define OLED_I2C_ADDRESS    0x3C
#else
#define OLED_I2C_ADDRESS    0x00
#endif

// Control byte
#define OLED_CONTROL_BYTE_CMD_SINGLE        0x80
#define OLED_CONTROL_BYTE_CMD_STREAM        0x00
#define OLED_CONTROL_BYTE_DATA_STREAM       0x40

#define LV_DISPLAY_DC_CMD_MODE    0
#define LV_DISPLAY_DC_DATA_MODE   1

/* Helper functions to get display communication interface kind, this can be
 * implemented as users see fit, we're using the symbols created by Kconfig
 * because is what we have available.
 * Other ways to implement it is using the user_data poiter in lv_disp_drv_t */
static inline bool display_interface_is_spi(lv_disp_drv_t * drv);
static inline bool display_interface_is_i2c(lv_disp_drv_t * drv);

void display_port_delay(lv_disp_drv_t *drv, uint32_t delay_ms)
{
    (void) drv;

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

void display_port_backlight(lv_disp_drv_t *drv, uint8_t state)
{
    (void) drv;

#ifdef CONFIG_LV_DISP_PIN_BCKL
    gpio_set_level(CONFIG_LV_DISP_PIN_BCKL, state);
#endif
}

void display_port_gpio_dc(lv_disp_drv_t *drv, uint8_t state)
{
    (void) drv;

#ifdef CONFIG_LV_DISPLAY_USE_DC
    gpio_set_level(CONFIG_LV_DISP_PIN_DC, state);
#endif
}

void display_port_gpio_rst(lv_disp_drv_t *drv, uint8_t state)
{
    (void) drv;

#ifdef CONFIG_LV_DISP_USE_RST
    gpio_set_level(CONFIG_LV_DISP_PIN_RST, state);
#endif
}

bool display_port_gpio_is_busy(lv_disp_drv_t *drv)
{
    (void) drv;

    bool device_busy = false;

#ifdef CONFIG_LV_DISP_PIN_BUSY
    /* FIXME Assuming the busy signal in logic 1 means the device is busy */
    if (gpio_get_level(CONFIG_LV_DISP_PIN_BUSY) == 1) {
        device_busy = true;
    }
#endif

    return device_busy;
}

void display_interface_send_cmd(lv_disp_drv_t *drv, uint32_t cmd, cmd_width_t cmd_width, void *args, size_t args_len)
{
    (void) drv;

    if (display_interface_is_spi(drv)) {
        disp_wait_for_pending_transactions();
        display_port_gpio_dc(drv, LV_DISPLAY_DC_CMD_MODE);

        if (CMD_WIDTH_8BITS == cmd_width) {
            uint8_t cmd_8bits = (uint8_t) cmd & 0xFFU;
            disp_spi_send_data(&cmd_8bits, 1);
        }
        else if (CMD_WIDTH_16BITS == cmd_width) {
            /* Send 16bits cmd */
        }
        else {
            /* Invalid cmd size */
            assert(0);
        }

        if (args != CMD_WITHOUT_ARGS) {
            display_port_gpio_dc(drv, LV_DISPLAY_DC_DATA_MODE);
            disp_spi_send_data(args, args_len);
        }
    }

    if (display_interface_is_i2c(drv)) {
        uint8_t *data = (uint8_t *) args;
        lvgl_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, cmd, data, args_len);
    }
}

void display_interface_send_data(lv_disp_drv_t *drv, void *data, size_t len)
{
    (void) drv;

    if (display_interface_is_spi(drv)) {
        disp_wait_for_pending_transactions();
        display_port_gpio_dc(drv, LV_DISPLAY_DC_DATA_MODE);
        disp_spi_send_colors(data, len);
        /* lv_disp_flush_ready is called in the SPI xfer done callback */
    }

    if (display_interface_is_i2c(drv)) {
        lvgl_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM, data, len);
    }
}

static inline bool display_interface_is_spi(lv_disp_drv_t * drv)
{
    (void) drv;

    bool retval = false;

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI)
    retval = true;
#endif

    return retval;
}

static inline bool display_interface_is_i2c(lv_disp_drv_t * drv)
{
    (void) drv;

    bool retval = false;

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_I2C)
    retval = true;
#endif

    return retval;
}
