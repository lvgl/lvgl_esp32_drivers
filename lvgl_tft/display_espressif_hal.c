#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_bsp.h"

#include "sdkconfig.h"
#include "driver/gpio.h"

void display_bsp_init_io(void)
{
#ifdef CONFIG_LV_DISPLAY_USE_DC
    gpio_pad_select_gpio(CONFIG_LV_DISP_PIN_DC);
    gpio_set_direction(CONFIG_LV_DISP_PIN_DC, GPIO_MODE_OUTPUT);
#endif

#ifdef CONFIG_LV_DISP_USE_RST
    gpio_pad_select_gpio(CONFIG_LV_DISP_PIN_RST);
    gpio_set_direction(CONFIG_LV_DISP_PIN_RST, GPIO_MODE_OUTPUT);
#endif

#ifdef CONFIG_LV_DISP_PIN_BCKL
    gpio_pad_select_gpio(CONFIG_LV_DISP_PIN_BCKL);
    gpio_set_direction(CONFIG_LV_DISP_PIN_BCKL, GPIO_MODE_OUTPUT);
#endif
}

void display_bsp_delay(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

void display_bsp_backlight(uint8_t state)
{
#ifdef CONFIG_LV_DISP_PIN_BCKL
    gpio_set_level(CONFIG_LV_DISP_PIN_BCKL, state);
#endif
}

void display_bsp_gpio_dc(uint8_t state)
{
#ifdef CONFIG_LV_DISPLAY_USE_DC
    gpio_set_level(CONFIG_LV_DISP_PIN_DC, state);
#endif
}

void display_bsp_gpio_rst(uint8_t state)
{
#ifdef CONFIG_LV_DISP_USE_RST
    gpio_set_level(CONFIG_LV_DISP_PIN_RST, state);
#endif
}
