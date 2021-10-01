#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_port.h"

#include "sdkconfig.h"
#include "driver/gpio.h"

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
