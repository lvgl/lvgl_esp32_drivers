/**
 * @file esp_lcd_backlight.h
 */

#ifndef ESP_LCD_BACKLIGHT_H
#define ESP_LCD_BACKLIGHT_H

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#ifdef __cplusplus
extern "C" { /* extern "C" */
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/*  Useful Backlight Constants for PWM control
//Backlight 

#define BL 27  
int dutyCycle;
const int PWMFreq = 2000; 
const int PWMChannel = 0;
const int PWMResolution = 10;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
dutyCycle=Lux;
if (dutyCycle> MAX_DUTY_CYCLE || dutyCycle <= 0){dutyCycle=MAX_DUTY_CYCLE;}
ledcWrite(PWMChannel, dutyCycle);

*/


/**
 * @brief Display backlight controller handle
 *
 */
typedef void * disp_backlight_h;

/**
 * @brief Configuration structure of backlight controller
 *
 * Must be passed to disp_backlight_new() for correct configuration
 */
typedef struct {
    bool pwm_control;
    bool output_invert;
    int gpio_num; // see gpio_num_t

    // Relevant only for PWM controlled backlight
    // Ignored for switch (ON/OFF) backlight control
    int timer_idx;   // ledc_timer_t
    int channel_idx; // ledc_channel_t
} disp_backlight_config_t;

/**
 * @brief Create new backlight controller
 *
 * @param[in] config Configuration structure of backlight controller
 * @return           Display backlight controller handle
 */
extern disp_backlight_h disp_backlight_new(const disp_backlight_config_t *config);

/**
 * @brief Set backlight
 *
 * Brightness parameter can be 0-100 for PWM controlled backlight.
 * GPIO controlled backlight (ON/OFF) is turned off witch value 0 and turned on with any positive value.
 *
 * @param bckl                   Backlight controller handle
 * @param[in] brightness_percent Brightness in [%]
 */
extern void disp_backlight_set(disp_backlight_h bckl, int brightness_percent);
extern void disp_backlight_delete(disp_backlight_h bckl);
void setDisplay(int BLvalue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ESP_LCD_BACKLIGHT_H*/
