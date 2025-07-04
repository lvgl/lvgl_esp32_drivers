/**
* @file ADCRAW.h
*/

#ifndef ADCRAW_H
#define ADCRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_USE_ESP_ADC		(0)
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"    /// IDF4.xx
#include "rom/gpio.h"   /// IDF 5.xx


#if TEST_USE_ESP_ADC
#include "esp_adc/adc_continuous.h"		/// IDF 5.xx
#else
#include "driver/adc.h"	/// IDF 4.xx

#endif
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
// #include "driver/gpio.h"
#include <stddef.h>
#include "freertos/timers.h"

#define portTICK_RATE_MS portTICK_PERIOD_MS  /// IDF 5.xx

extern bool stoptimer;
extern bool landscapeadc;
extern bool touched;
extern bool touchreset;
extern uint8_t touchcount;

extern TimerHandle_t SWTIMER;
#define TOUCHSCREEN_RESISTIVE_PIN_YU 19 // Y+ any gpio
#define TOUCHSCREEN_RESISTIVE_PIN_YD 33 // Y- also ADC
#define TOUCHSCREEN_RESISTIVE_PIN_XL 5 // X- any gpio
#define TOUCHSCREEN_RESISTIVE_PIN_XR 32 // X+ also ADC

// Default calibration points
#define TOUCHCAL_ULX 29  // Upper Left X
#define TOUCHCAL_ULY 84  // Upper Left Y
#define TOUCHCAL_URX 828 // Upper Right X
#define TOUCHCAL_URY 60  // Upper Right Y
#define TOUCHCAL_LLX 29  // Lower Left X
#define TOUCHCAL_LLY 928 // Lower Left Y
#define TOUCHCAL_LRX 828 // Lower Right X
#define TOUCHCAL_LRY 928 // Lower Right Y

#define TOUCHSCREEN_RESISTIVE_PRESS_THRESHOLD 1023



extern bool isinverted_adcraw;
	
/*GetMaxX Macro*/
#if CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE
#define GetMaxX()   (LV_HOR_RES_MAX - 1)
#else
#define GetMaxX()   (LV_VER_RES_MAX - 1)
#endif

/*GetMaxY Macro*/
#if CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE
#define GetMaxY()   (LV_VER_RES_MAX - 1)
#else
#define GetMaxY()   (LV_HOR_RES_MAX - 1)
#endif

#ifndef CONCAT3
#define _CONCAT3(a,b,c) a ## b ## c
#define CONCAT3(a,b,c) _CONCAT3(a,b,c)
#endif

#define GPIO_TO_ADC_ELEMENT(x) [x] = CONCAT3(ADC1_GPIO, x, _CHANNEL)

typedef enum {
    IDLE,
    SET_X,
    SET_Y,
    SET_Z1,
    SET_Z2,
	READ_X,
	READ_Y,
	READ_Z1,
	READ_Z2
} TOUCH_STATES; 

extern int adcX, adcY,adcX2,adcY2;



void adcraw_init(void);
bool adcraw_read(lv_indev_drv_t * drv, lv_indev_data_t * data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ADCRAW_H */
