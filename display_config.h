#ifndef DISPLAY_CONFIG_H_
#define DISPLAY_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"

/* Configuration options for ST7789 display controllers */
#if CONFIG_LV_DISP_USE_RST
  #if CONFIG_LV_DISP_ST7789_SOFT_RESET
    #define ST7789_SOFT_RST
  #endif
#else
  #define ST7789_SOFT_RST
#endif

#if defined (CONFIG_LV_INVERT_COLORS)
#define ST7789_INVERT_COLORS    1U
#endif
    
#define ST7789_INITIAL_ORIENTATION  CONFIG_LV_DISPLAY_ORIENTATION

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DISPLAY_CONFIG_H_ */
