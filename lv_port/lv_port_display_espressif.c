#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_port.h"

#include "sdkconfig.h"
#include "driver/gpio.h"

#include "disp_spi.h"
#include "lvgl_i2c/i2c_manager.h"

/* TODO: This is ssd1306 specific */
#define OLED_I2C_PORT                       (CONFIG_LV_I2C_DISPLAY_PORT)
#define OLED_I2C_ADDRESS                    0x3C

#define LV_DISPLAY_DC_CMD_MODE    0
#define LV_DISPLAY_DC_DATA_MODE   1

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

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI)
    disp_wait_for_pending_transactions();
    display_port_gpio_dc(drv, LV_DISPLAY_DC_CMD_MODE);

    if (CMD_WIDTH_8BITS == cmd_width) {
        disp_spi_send_data(&cmd, 1);
    }
    else if (CMD_WIDTH_16BITS == cmd_width) {
        /* Send 16bits cmd */
    }
    else {
        /* Unsupported cmd size */
    }

    if (args != NULL) {
        display_port_gpio_dc(drv, LV_DISPLAY_DC_DATA_MODE);
        disp_spi_send_data(args, args_len);
    }
#elif defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_I2C)
    uint8_t *data = (uint8_t *) args;
    
    lvgl_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, cmd, data, args_len); 
#endif
}

void display_interface_send_data(lv_disp_drv_t *drv, void *data, size_t len)
{
    (void) drv;

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI)
    disp_wait_for_pending_transactions();
    display_port_gpio_dc(drv, LV_DISPLAY_DC_DATA_MODE);
    disp_spi_send_colors(data, len);
    /* lv_disp_flush is called in the SPI xfer done callback */
#elif defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_I2C)
    lvgl_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM, data, len);
#endif
}
