/**
 * @file ADCRAW.c
 */

#include "adcraw.h"
#include "esp_system.h"
#include "esp_log.h"

//Mycode
#include "soc/adc_channel.h"
#include "soc/soc.h"
#include "soc/soc_memory_layout.h"


/*
Touch position accuacy
The coordinates read from Y(+) and X(-) are analog values.
The difference between the coordinates read last time and the coordinates read this time is determined, and if it is within this range, it is regarded as a valid coordinate.
Decreasing this value will make the position more accurate, but less responsive.
Increasing this value will make the position more inaccurate but more responsive.
*/


#define TAG "ADCRAW"
#define CALIBRATIONINSET 1 // range 0 <= CALIBRATIONINSET <= 40
#define SAMPLE_CALIBRATION_POINTS 4
// use this scale factor to avoid working in floating point numbers
#define TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR 8
#define SCALE_FACTOR (1 << TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR)
#define CAL_X_INSET (((GetMaxX() + 1) * (CALIBRATIONINSET >> 1)) / 100)
#define CAL_Y_INSET (((GetMaxY() + 1) * (CALIBRATIONINSET >> 1)) / 100)
#define NUMSAMPLES 2


#define ADCTESTING 0

#define CONFIG_LV_TOUCH_XY_SWAP true 

bool isinverted_adcraw = true;
bool stoptimer = false;
bool landscapeadc = false;
uint8_t touchcount = 0;
static void ad_touch_handler(void *arg);

//ESP HW TIMER OLD
// static const esp_timer_create_args_t periodic_timer_args = {
// 	.callback = &ad_touch_handler,
// };
// static esp_timer_handle_t periodic_timer;


//SOFTWARE TIMER TEST
TimerHandle_t SWTIMER = 0;
StaticTimer_t xtimmerbuffer;

// Current ADC values for X and Y channels
int adcX, adcY,adcX2,adcY2 = 0;

int temp_x, temp_y, temp_z1, temp_z2;
//int16_t
bool touched = false;
bool touchreset = false;
// coefficient values
int _trA, _trB, _trC, _trD;

int16_t xRawTouch[SAMPLE_CALIBRATION_POINTS];
int16_t yRawTouch[SAMPLE_CALIBRATION_POINTS];
TOUCH_STATES state;

const gpio_num_t yu = TOUCHSCREEN_RESISTIVE_PIN_YU;
const gpio_num_t xl = TOUCHSCREEN_RESISTIVE_PIN_XL;
const gpio_num_t yd = 33;//TOUCHSCREEN_RESISTIVE_PIN_YD; //ADC
const gpio_num_t xr = 32;//TOUCHSCREEN_RESISTIVE_PIN_XR;  //ADC

#define ADC1_CHANNEL_5 0x05
#define ADC1_CHANNEL_4 0x04

#if TEST_USE_ESP_ADC
/// @brief it may use adc_continuous to implement these...
/// @param channel 
/// @return 
uint16_t adc1_get_raw(uint8_t channel){
	return 0;
}
#else
void adc_gpio_init(uint8_t unit, uint8_t channel){
	/// dummy no longer used.
}

static const int gpio_to_adc[] = {
	// GPIO_TO_ADC_ELEMENT(TOUCHSCREEN_RESISTIVE_PIN_YD),
	// GPIO_TO_ADC_ELEMENT(TOUCHSCREEN_RESISTIVE_PIN_XR), 
	// ADC2_CHANNEL_0,ADC2_CHANNEL_2
	ADC1_CHANNEL_5,ADC1_CHANNEL_4
};
#endif

static void TouchCalculateCalPoints(void)
{
	int32_t trA, trB, trC, trD;     // variables for the coefficients
	int32_t trAhold, trBhold, trChold, trDhold;
	int32_t test1, test2;           // temp variables (must be signed type)

	int16_t xPoint[SAMPLE_CALIBRATION_POINTS], yPoint[SAMPLE_CALIBRATION_POINTS];

	yPoint[0] = yPoint[1] = CAL_Y_INSET;
	yPoint[2] = yPoint[3] = (GetMaxY() - CAL_Y_INSET);
	xPoint[0] = xPoint[3] = CAL_X_INSET;
	xPoint[1] = xPoint[2] = (GetMaxX() - CAL_X_INSET);

	// calculate points transfer function
	// based on two simultaneous equations solve for the constants

	// use sample points 1 and 4
	// Dy1 = aTy1 + b; Dy4 = aTy4 + b
	// Dx1 = cTx1 + d; Dy4 = aTy4 + b

	test1 = (int32_t)yPoint[0] - (int32_t)yPoint[3];
	test2 = (int32_t)yRawTouch[0] - (int32_t)yRawTouch[3];

	trA = ((int32_t)((int32_t)test1 * SCALE_FACTOR) / test2);
	trB = ((int32_t)((int32_t)yPoint[0] * SCALE_FACTOR) - (trA * (int32_t)yRawTouch[0]));

	test1 = (int32_t)xPoint[0] - (int32_t)xPoint[2];
	test2 = (int32_t)xRawTouch[0] - (int32_t)xRawTouch[2];

	trC = ((int32_t)((int32_t)test1 * SCALE_FACTOR) / test2);
	trD = ((int32_t)((int32_t)xPoint[0] * SCALE_FACTOR) - (trC * (int32_t)xRawTouch[0]));

	trAhold = trA;
	trBhold = trB;
	trChold = trC;
	trDhold = trD;

	// use sample points 2 and 3
	// Dy2 = aTy2 + b; Dy3 = aTy3 + b
	// Dx2 = cTx2 + d; Dy3 = aTy3 + b

	test1 = (int32_t)yPoint[1] - (int32_t)yPoint[2];
	test2 = (int32_t)yRawTouch[1] - (int32_t)yRawTouch[2];

	trA = ((int32_t)(test1 * SCALE_FACTOR) / test2);
	trB = ((int32_t)((int32_t)yPoint[1] * SCALE_FACTOR) - (trA * (int32_t)yRawTouch[1]));

	test1 = (int32_t)xPoint[1] - (int32_t)xPoint[3];
	test2 = (int32_t)xRawTouch[1] - (int32_t)xRawTouch[3];

	trC = ((int32_t)((int32_t)test1 * SCALE_FACTOR) / test2);
	trD = ((int32_t)((int32_t)xPoint[1] * SCALE_FACTOR) - (trC * (int32_t)xRawTouch[1]));

	// get the average and use it
	_trA = (trA + trAhold) >> 1;
	_trB = (trB + trBhold) >> 1;
	_trC = (trC + trChold) >> 1;
	_trD = (trD + trDhold) >> 1;
}

void adcraw_init(void)
{
	state = IDLE;      // set the state of the state machine to start the sampling
	gpio_set_drive_capability(yu, GPIO_DRIVE_CAP_3);
	gpio_set_drive_capability(33, GPIO_DRIVE_CAP_3);
	gpio_set_drive_capability(xl, GPIO_DRIVE_CAP_3);
	gpio_set_drive_capability(32, GPIO_DRIVE_CAP_3);
	//ESP HARDWARE Timer Begin + Wait	
	// (esp_timer_create(&periodic_timer_args, &periodic_timer));
	// ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 5 * 1000));        //5ms (expressed as microseconds)
	
	//Timer SW TEST
	SWTIMER = xTimerCreate("ADCTEST", pdMS_TO_TICKS(20),true,1, ad_touch_handler); 
	//SWTIMER = xTimerCreate("ADCTEST", pdMS_TO_TICKS(20),true,4, ad_touch_handler); 
	xTimerStart(SWTIMER,0);

	/*Load calibration data*/
	xRawTouch[0] = TOUCHCAL_ULX;
	yRawTouch[0] = TOUCHCAL_ULY;
	xRawTouch[1] = TOUCHCAL_URX;
	yRawTouch[1] = TOUCHCAL_URY;
	xRawTouch[3] = TOUCHCAL_LLX;
	yRawTouch[3] = TOUCHCAL_LLY;
	xRawTouch[2] = TOUCHCAL_LRX;
	yRawTouch[2] = TOUCHCAL_LRY;

	TouchCalculateCalPoints();
}
void adcraw_set_timer(bool en){


}
static void setup_axis(gpio_num_t plus, gpio_num_t minus, gpio_num_t measure, gpio_num_t ignore)
{
	// Set GPIOs:
	// - Float "ignore" and "measure"
	gpio_pad_select_gpio(ignore);
	gpio_set_direction(ignore, GPIO_MODE_DISABLE);
	gpio_set_pull_mode(ignore, GPIO_PULLDOWN_ONLY);
	//GPIO_PULLUP_ONLY   OK= GPIO_PULLDOWN_ONLY
	//gpio_set_pull_mode(ignore, GPIO_FLOATING);
	gpio_pad_select_gpio(measure);
	gpio_set_direction(measure, GPIO_MODE_DISABLE);
	//gpio_set_pull_mode(measure, GPIO_FLOATING);
	gpio_set_pull_mode(measure, GPIO_PULLDOWN_ONLY);
	// - Set "plus" to 1, "minus" to 0
	gpio_config(&(gpio_config_t) {
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = (1ULL << plus) | (1ULL << minus)
	});
	gpio_set_level(plus, 1);
	gpio_set_level(minus, 0);
}

static void setup_adc(gpio_num_t measure)
{
	// Init ADC
	adc1_channel_t channel = gpio_to_adc[measure];

	adc_gpio_init(ADC_UNIT_1, channel);
	adc1_config_width(ADC_WIDTH_BIT_10);
	adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);

}
static void setup_adc_y(gpio_num_t measure)
{
	//Init ADC
	adc1_channel_t channel = 5;
	adc1_config_width(ADC_WIDTH_BIT_10);
	adc_gpio_init(ADC_UNIT_1, channel);
	adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);

}

static void setup_adc_x(gpio_num_t measure)
{
	// Init ADC
	adc1_channel_t channel = 4;
	adc1_config_width(ADC_WIDTH_BIT_10);
	adc_gpio_init(ADC_UNIT_1, channel);
	adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);
}
    
static void insert_sort(int16_t array[], uint8_t size) {
	uint8_t j;
	int16_t save;

	for (int i = 1; i < size; i++) {
		save = array[i];
		for (j = i; j >= 1 && save < array[j - 1]; j--)
			array[j] = array[j - 1];
		array[j] = save;
	}
}

//Some switch states have no breaks, due to LVGL's ADCRAW logic please refer to
//https://github.com/lvgl/lvgl_esp32_drivers/blob/master/lvgl_touch/adcraw.c
static void ad_touch_handler(void *arg)
{
	(void) arg;
	uint8_t i;
	int16_t samples[NUMSAMPLES];
	//esp_err_t r;
	//int raw;
	switch (state) {
	case IDLE:
		adcX = 0;
		adcY = 0;
	case SET_X :
		setup_axis(yd, yu, xr, xl);
		setup_adc_x(xr);
		state = READ_X;
		break;
	case READ_X:
		for (i = 0; i < NUMSAMPLES; i++){
			// adc2_get_raw(2,1,&adcX);
			// samples[i] = adcX;
			samples[i] = adc1_get_raw(ADC1_CHANNEL_4);
		}
		insert_sort(samples, NUMSAMPLES);
		temp_x = samples[NUMSAMPLES / 2];
	case SET_Y :
		setup_axis(xl, xr, yd, yu);
		setup_adc_y(yd);
		state = READ_Y;
		break;
	case READ_Y:
		for (i = 0; i < NUMSAMPLES; i++){
			//adc2_get_raw(0,1,&adcY);
			//samples[i] = adcY;
			samples[i] = adc1_get_raw(ADC1_CHANNEL_5);
		}
		insert_sort(samples, NUMSAMPLES);
		temp_y = samples[NUMSAMPLES / 2];
	case SET_Z1 :
		setup_axis(yu, xl, yd, xr);
		setup_adc_y(yd);
		state = READ_Z1;
		break;
	case READ_Z1:
		// adc2_get_raw(0,1,&adcY);
		// temp_z1 = adcY;
		temp_z1 = adc1_get_raw(ADC1_CHANNEL_5);
	case SET_Z2 :
		setup_axis(yu, xl, xr, yd);
		setup_adc_y(yd);
		state = READ_Z2;
		break;
	case READ_Z2:
		// adc2_get_raw(2,1,&adcX);
		// temp_z2 = adcX;
		temp_z2 = adc1_get_raw(ADC1_CHANNEL_4);
		
		if (temp_z1 < TOUCHSCREEN_RESISTIVE_PRESS_THRESHOLD) {
#if CONFIG_LV_TOUCH_XY_SWAP
			
			adcY = adcY2 = temp_x; 
			adcX = adcX2 = temp_y; 
			touchreset = touched = true;
			touchcount++;
			// printf("Touched x: %d   y: %d \n", adcX, adcY);
#if ADCTESTING
			printf("Touched x: %d   y: %d \n", adcX, adcY);
			printf("Z1: %d  THRESHOLD: %d \n", temp_z1, TOUCHSCREEN_RESISTIVE_PRESS_THRESHOLD );
#endif

#else
			adcY = adcY2 = temp_y; 
			adcX = adcX2 = temp_x; 
			touchreset =touched = true;
#endif
#if ADCTESTING
			printf("Touched x: %d   y: %d \n", adcX, adcY);
#endif
		}
		else {
#if ADCTESTING
			printf("Not touched, z1: %d  THRESHOLD: %d \n", temp_z1, TOUCHSCREEN_RESISTIVE_PRESS_THRESHOLD );
#endif			
			touched = false;
			adcX = adcX2 = -1; 
			adcY = adcY2 = -1; 
		}
		state = SET_X;
#if ADCTESTING
		printf("Final Values, x: %d   y: %d   z: %d\n", adcX, adcY, temp_z1 - temp_z2);
#endif
		break;
	}

	return;
}

static int16_t TouchGetRawX(void)
{
	int16_t x = adcX2;
	// if(x == adcY){
	// 	printf("x: %d == y %d\n",adcX,adcY);
	// 	x = adcX;
	// }
#if ADCTESTING
    printf("RawX b4 invert: %d adcX2 %d\n",x, adcX);
#endif	
// #if CONFIG_LV_TOUCH_INVERT_X
// 	x = 1023 - x;
// #endif
	if( isinverted_adcraw == true){
		x = 1023 - x;
	}

	return x;
}

static int16_t TouchGetX(void)
{
	int16_t result = TouchGetRawX();
	if(touched == false){
		result = -1;
		return (result);
	}
#if ADCTESTING
	printf("X result before scale: %d ,Final ", result);
#endif

	if (result > 0) {
#if CONFIG_LV_TOUCH_XY_SWAP

			result = (int16_t)((((int32_t)_trA * result) + (int32_t)_trB) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
			//result = (int16_t)((((int32_t)_trC * result) + _trD) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
#else
			result = (int16_t)((((int32_t)_trC * result) + _trD) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);

#endif
		//result = (int16_t)((((int32_t)_trC * result) + _trD) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
	}
#if ADCTESTING
	printf("got x: %d\n", result);
#endif
	return (result);
}

static int16_t TouchGetRawY(void)
{
	int16_t y = adcY2;
#if ADCTESTING
	printf("TouchGetRawY before invert: %d\n", y);
#endif
// #if CONFIG_LV_TOUCH_INVERT_Y
// 	y = 1023 - y;
// #endif

	if( isinverted_adcraw == true){
		y = 1023 - y;
	}
	return y;
}

static int16_t TouchGetY(void)
{
	int16_t result = TouchGetRawY();
	if(touched == false){
		result = -1;
		return (result);
	}
#if ADCTESTING
	printf("Y result before scale: %d, Final ", result);
#endif
	if (result > 0) {
#if CONFIG_LV_TOUCH_XY_SWAP
		result = (int16_t)((((int32_t)_trC * result) + _trD) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
		//result = (int16_t)((((int32_t)_trA * result) + (int32_t)_trB) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
#else
		result = (int16_t)((((int32_t)_trA * result) + (int32_t)_trB) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
#endif
		//result = (int16_t)((((int32_t)_trA * result) + (int32_t)_trB) >> TOUCHSCREEN_RESISTIVE_CALIBRATION_SCALE_FACTOR);
	}
#if ADCTESTING
	printf("get y: %d\n", result);
#endif
	return (result);
}

/**
 * Get the current position and state of the touchpad
 * @param data store the read data here
 * @return false: because no more data to be read
 */
bool adcraw_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
	static int16_t last_x = 0;
	static int16_t last_y = 0;

	int16_t x, y;

	// x = TouchGetX();
	// y = TouchGetY();

	x = TouchGetX();
	//wierd offset...?
	//y = TouchGetY();
	if(landscapeadc){
		y = TouchGetY();  
		if(isinverted_adcraw){
			y = y - 40;
		}
	}else{
		y = TouchGetY()-30;  
	}
	
	if(y < 0){
		y = 0;
	} 
	if(x < 0){
		x = 0;
	}

	// if ((x > 0) && (y > 0)) {
	// 	data->point.x = x;
	// 	data->point.y = y;
	// 	last_x = data->point.x;
	// 	last_y = data->point.y;
	// 	data->state = LV_INDEV_STATE_PR;
	// }
	// else {
	// 	data->point.x = last_x;
	// 	data->point.y = last_y;
	// 	data->state = LV_INDEV_STATE_REL;
	// }

	if ((x > 0) && (y > 0) && touched == true) {
		data->point.x = x;
		data->point.y = y;
		last_y = data->point.x;
		last_x = data->point.y;
		data->state = LV_INDEV_STATE_PR;
	}
	else if (touched == true){
		data->point.x = last_x;
		data->point.y = last_y;
		data->state = LV_INDEV_STATE_REL;
	}
	else {
		data->point.x = -1;
		data->point.y = -1;
		data->state = LV_INDEV_STATE_REL;
	}
	return false;
}



//#endif
