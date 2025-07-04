/**
 * @file touch_driver.c
 */

#include "touch_driver.h"
#include "tp_spi.h"

uint8_t touchtype = RESISTIVE_TOUCH;
void touch_driver_init(void)
{

    //Approach one, using the ALS, start either
    //Initialize Touch Driver based on type detective, default is resistive
    // if(touchtype == RESISTIVE_TOUCH){
    //     printf("Inititializing Resisitive touch screen...\n");
    //     adcraw_init();
    // }else{
    //     printf("Inititializing Capacitive touch screen...\n");
    //     gt911_init(GT911_I2C_SLAVE_ADDR);
    //     printf("Capacitive Value %s - %d",gt911_status.inited ? "Initialized" : "Failed" , gt911_status.inited);
    // }    

    //Approach two, if GT911 is not initialized, use ADC RAW
    printf("Inititializing Capacitive touch screen...\n");
    gt911_init(GT911_I2C_SLAVE_ADDR);
    printf("Capacitive Value %s - %d",gt911_status.inited ? "Initialized" : "Failed" , gt911_status.inited);

    if(gt911_status.inited  == false){
        printf("Using ADV_RAW touch\n");
        adcraw_init();
    }



// #if defined (CONFIG_LV_TOUCH_CONTROLLER_XPT2046)
//     xpt2046_init();
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_FT6X06)
//     ft6x06_init(FT6236_I2C_SLAVE_ADDR);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_STMPE610)
//     stmpe610_init();
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_ADCRAW)
//     adcraw_init();
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_FT81X)
//     /* nothing to do */
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_RA8875)
//     ra8875_touch_init();
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_GT911)
//     gt911_init(GT911_I2C_SLAVE_ADDR);
// #endif
}

#if LVGL_VERSION_MAJOR >= 8
void touch_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
#else
bool touch_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
#endif
{
    bool res = false;
    if(touchtype == RESISTIVE_TOUCH){
        res = adcraw_read(drv, data);
    }else{
        res = gt911_read(drv, data);
    }
    
// #if defined (CONFIG_LV_TOUCH_CONTROLLER_XPT2046)
//     res = xpt2046_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_FT6X06)
//     res = ft6x36_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_STMPE610)
//     res = stmpe610_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_ADCRAW)
//     res = adcraw_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_FT81X)
//     res = FT81x_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_RA8875)
//     res = ra8875_touch_read(drv, data);
// #elif defined (CONFIG_LV_TOUCH_CONTROLLER_GT911)
//     res = gt911_read(drv, data);
// #endif

#if LVGL_VERSION_MAJOR >= 8
    data->continue_reading = res;
#else
    return res;
#endif
}

//If display is capactive, timer will crash display
void resistive_disable(){ 
    if(touchtype == RESISTIVE_TOUCH){ 
        if(SWTIMER)
        xTimerStop(SWTIMER,0); 
    } 
} 
 
void resistive_enable(){ 
    if(touchtype == RESISTIVE_TOUCH){
        if(SWTIMER) 
        xTimerStart(SWTIMER,0); 
    } 
} 