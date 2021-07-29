#ifndef DISPLAY_BSP_H_
#define DISPLAY_BSP_H_

void display_bsp_delay(uint32_t delay_ms);
void display_bsp_backlight(uint8_t state);
void display_bsp_gpio_dc(uint8_t state);
void display_bsp_gpio_rst(uint8_t state);

#endif
