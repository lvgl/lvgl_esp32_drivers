/**
 * @file ssd1680.h
 *
 */

#ifndef SSD1680_H
#define SSD1680_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "sdkconfig.h"

#define EPD_PANEL_WIDTH          LV_HOR_RES_MAX   /* 128 */
#define EPD_PANEL_HEIGHT         LV_VER_RES_MAX  /* 296 */

/* 128 = panel width */
#define SSD1680_COLUMNS          (EPD_PANEL_WIDTH / 8)

#define SSD1680_DC_PIN           CONFIG_LV_DISP_PIN_DC
#define SSD1680_RST_PIN          CONFIG_LV_DISP_PIN_RST
#define SSD1680_USE_RST          CONFIG_LV_DISP_USE_RST
#define SSD1680_BUSY_PIN         CONFIG_LV_DISP_PIN_BUSY
#define SSD1680_BUSY_LEVEL       1 //chip is busy if the pin level is high

/* SSD1680 commands */
#define SSD1680_CMD_GDO_CTRL			0x01 //Driver output control
#define SSD1680_CMD_GDV_CTRL			0x03
#define SSD1680_CMD_SDV_CTRL			0x04
#define SSD1680_CMD_SOFTSTART			0x0c
#define SSD1680_CMD_GSCAN_START			0x0f

#define SSD1680_CMD_SLEEP_MODE1			0x10 //enter deep sleep 1 - retain RAM
#define SSD1680_CMD_SLEEP_MODE2         0x11 //enter deep sleep 2 - cannot retain RAM
//After entering Deep sleep Mode, BUSY will stay high.
//To Exit Deep Sleep a HWRESET should be sent.

#define SSD1680_CMD_ENTRY_MODE			0x11 //Data entry mode
#define SSD1680_CMD_SW_RESET			0x12 //SWRESET
#define SSD1680_CMD_TSENS_CTRL			0x1a
#define SSD1680_CMD_READ_INT_TEMP       0x18 //Read built-in temperature sensor
#define SSD1680_CMD_MASTER_ACTIVATION	0x20 //Activate Display Update Sequence
#define SSD1680_CMD_UPDATE_CTRL1		0x21 //Display update control
#define SSD1680_CMD_UPDATE_CTRL2		0x22 //Display Update Control
#define SSD1680_CMD_WRITE1_RAM			0x24 //Write Black (0) and Wite (1) image to RAM
#define SSD1680_CMD_WRITE2_RAM		    0x26 //Write RED (1) and NonRED (0) image to RAM
#define SSD1680_CMD_VCOM_SENSE			0x28
#define SSD1680_CMD_VCOM_SENSE_DURATON	0x29
#define SSD1680_CMD_PRGM_VCOM_OTP		0x2a
#define SSD1680_CMD_VCOM_VOLTAGE		0x2c
#define SSD1680_CMD_PRGM_WS_OTP			0x30
#define SSD1680_CMD_UPDATE_LUT			0x32
#define SSD1680_CMD_PRGM_OTP_SELECTION	0x36
#define SSD1680_CMD_WRITE_DISPL_OPT 	0x37
#define SSD1680_CMD_BWF_CTRL			0x3c //BorderWavefrom
#define SSD1680_CMD_RAM_XPOS_CTRL		0x44 //set Ram-X address start/end position
#define SSD1680_CMD_RAM_YPOS_CTRL		0x45 //set Ram-Y address start/end position
#define SSD1680_CMD_RAM_XPOS_CNTR		0x4e //set RAM x address count to 0;
#define SSD1680_CMD_RAM_YPOS_CNTR		0x4f //set RAM y address count to 0X199;

/* Data entry sequence modes */
#define SSD1680_DATA_ENTRY_MASK			0x07
#define SSD1680_DATA_ENTRY_XDYDX		0x00
#define SSD1680_DATA_ENTRY_XIYDX		0x01
#define SSD1680_DATA_ENTRY_XDYIX		0x02
#define SSD1680_DATA_ENTRY_XIYIX		0x03
#define SSD1680_DATA_ENTRY_XDYDY		0x04
#define SSD1680_DATA_ENTRY_XIYDY		0x05
#define SSD1680_DATA_ENTRY_XDYIY		0x06
#define SSD1680_DATA_ENTRY_XIYIY		0x07

/* Options for display update */
#define SSD1680_CTRL1_INITIAL_UPDATE_LL	0x00
#define SSD1680_CTRL1_INITIAL_UPDATE_LH	0x01
#define SSD1680_CTRL1_INITIAL_UPDATE_HL	0x02
#define SSD1680_CTRL1_INITIAL_UPDATE_HH	0x03

/* Options for display update sequence */
#define SSD1680_CTRL2_ENABLE_CLK		0x80
#define SSD1680_CTRL2_ENABLE_ANALOG		0x40
#define SSD1680_CTRL2_TO_INITIAL		0x08
#define SSD1680_CTRL2_TO_PATTERN		0x04
#define SSD1680_CTRL2_DISABLE_ANALOG	0x02
#define SSD1680_CTRL2_DISABLE_CLK		0x01

#define SSD1680_SLEEP_MODE_DSM			0x01
#define SSD1680_SLEEP_MODE_PON			0x00

/* time constants in ms */
#define SSD1680_RESET_DELAY			    20 //At least 10ms delay
#define SSD1680_BUSY_DELAY			    1
// normal wait time max 20 times x 10ms
#define SSD1680_WAIT                    20

void ssd1680_init(void);
void ssd1680_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void ssd1680_rounder(lv_disp_drv_t * disp_drv, lv_area_t *area);
void ssd1680_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);

void ssd1680_deep_sleep(void);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __SSD1680_REGS_H__ */
