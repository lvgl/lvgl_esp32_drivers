/**
 * @file esp_lcd_backlight.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "esp_lcd_backlight.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/ledc_periph.h" // to invert LEDC output on IDF version < v4.3

typedef struct {
    bool pwm_control; // true: LEDC is used, false: GPIO is used
    int index;        // Either GPIO or LEDC channel
} disp_backlight_t;

static const char *TAG = "disp_backlight";


void setDisplay(int BLvalue){
	//uint32_t blconvert = BLvalue;
	uint32_t dutyraw = 255;

	if(BLvalue > 100){
		dutyraw = dutyraw;
	}else{
		dutyraw = (float)(dutyraw*BLvalue/100);
	}
	// #if !RELEASE
	//   printf("duty raw is %d, %d \n",dutyraw, BLvalue);
	// #endif
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, 0, dutyraw));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, 0));
	
	//printf("before FREQ");
	//edc_set_freq(LEDC_HIGH_SPEED_MODE,PWM_LGVL.timer_idx,20000);
	// printf("\n current freq %d \n",ledc_get_freq(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_0));
	//ESP_LOGI(TAG, "NEW DUTY VALUE IS %f BL VALUE GOT WAS %d other value.. %f",dutyvalue,BLvalue, dutyraw);
}



void disp_backlight_set(disp_backlight_h bckl, int brightness_percent)
{
    // Check input paramters
    if (bckl == NULL)
        return;
    if (brightness_percent > 100)
        brightness_percent = 100;
    if (brightness_percent < 0)
        brightness_percent = 0;

    disp_backlight_t *bckl_dev = (disp_backlight_t *) bckl;
    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);

    if (bckl_dev->pwm_control) {
        uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, bckl_dev->index, duty_cycle));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, bckl_dev->index));
    } else {
        ESP_ERROR_CHECK(gpio_set_level(bckl_dev->index, brightness_percent));
    }
}

void disp_backlight_delete(disp_backlight_h bckl)
{
    if (bckl == NULL)
        return;

    disp_backlight_t *bckl_dev = (disp_backlight_t *) bckl;
    if (bckl_dev->pwm_control) {
        ledc_stop(LEDC_LOW_SPEED_MODE, bckl_dev->index, 0);
    } else {
        gpio_reset_pin(bckl_dev->index);
    }
    free (bckl);
}
