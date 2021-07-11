/**
 * @file esp_lcd_backlight.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "esp_lcd_backlight.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "disp_brightness";

void disp_brightness_control_enable(void)
{
	/*
		Configure LED (Backlight) pin as PWM for Brightness control.
	*/
    ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = DISP_PIN_BCKL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = 0
    };
    ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .bit_num = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

	ESP_ERROR_CHECK( ledc_timer_config(&LCD_backlight_timer) );
	ESP_ERROR_CHECK( ledc_channel_config(&LCD_backlight_channel) );

}

void disp_set_brightness(uint16_t brightness)
{
	/*
		Set brightness.
		0   -> Display off
		100 -> Full brightness
		NOTE: brightness value must be between 0 - 100
	*/
	if(brightness > 100)
	{
		ESP_LOGE(TAG, "Brightness value must be between 0 - 100");
		return;
	}
    ESP_ERROR_CHECK( ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness*10) );
	ESP_ERROR_CHECK( ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) );
}
