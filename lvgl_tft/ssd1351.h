/**
 * @file lv_templ.h
 *
 */

#ifndef SSD1351_H
#define SSD1351_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "../lvgl_helpers.h"

/*********************
 *      DEFINES
 *********************/
#define SSD1351_DC CONFIG_LV_DISP_PIN_DC
#define SSD1351_RST CONFIG_LV_DISP_PIN_RST

// default orientation
#define SSD1351_WIDTH 128
#define SSD1351_HEIGHT 128
/*********************
 *     COMMANDS
 *********************/

// Color definitions
#define SSD1351_BLACK   0x0000
#define SSD1351_BLUE    0x001F
#define SSD1351_RED     0xF800
#define SSD1351_GREEN   0x07E0
#define SSD1351_CYAN    0x07FF
#define SSD1351_MAGENTA 0xF81F
#define SSD1351_YELLOW  0xFFE0
#define SSD1351_WHITE   0xFFFF
#define SSD1351_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

enum ESsd1351Commands {
  SSD1351_COLUMNADDR = 0x15,
  SSD1331_WRITEDATA = 0x5C,
  SSD1351_ROWADDR = 0x75,
  SSD1351_SEGREMAP = 0xA0,
  SSD1351_SETSTARTLINE = 0xA1,
  SSD1351_SETDISPLAYOFFSET = 0xA2,
  SSD1351_SETFUNCTION = 0xAB,
  SSD1351_NOP = 0xAD,
  SSD1351_ALLOFF = 0xA4,
  SSD1351_DISPLAYON = 0xA5,
  SSD1351_NORMALDISPLAY = 0xA6,
  SSD1351_DISPLAYINVERSE = 0xA7,
  SSD1351_SLEEP_ON = 0xAE,
  SSD1351_SLEEP_OFF = 0xAF,
  SSD1351_NOP2 = 0xB0,
  SSD1351_SETPRECHARGE = 0xB1,
  SSD1351_CLOCKDIV = 0xB3,
  SSD1351_EXTVSL = 0xB4,
  SSD1351_SETGPIO = 0xB5,
  SSD1351_PRECHARGESECOND = 0xB6,
  SSD1351_PRECHARGELEVEL = 0xBB,
  SSD1351_VCOMH = 0xBE,
  SSD1351_CONTRAST = 0xC1,
  SSD1351_MASTERCURRENT = 0xC7,
  SSD1351_SETMULTIPLEX = 0xCA,
  SSD1351_PRECHARGEA = 0x8A,
  SSD1351_PRECHARGEB = 0x8B,
  SSD1351_NOP3 = 0xE3,
  SSD1351_UNLOCK = 0xFD,
};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void ssd1351_init(void);
void ssd1351_flush(lv_disp_drv_t *drv, const lv_area_t *area,
                   lv_color_t *color_map);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SSD1351_H*/
