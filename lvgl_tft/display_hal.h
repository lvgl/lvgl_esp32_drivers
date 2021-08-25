#ifndef DISPLAY_HAL_H_
#define DISPLAY_HAL_H_

void display_hal_init_io(void);
void display_hal_delay(uint32_t delay_ms);
void display_hal_backlight(uint8_t state);
void display_hal_gpio_dc(uint8_t state);
void display_hal_gpio_rst(uint8_t state);

#endif
