/**
@file ssd1680.c
@brief   Tested with GoodDisplay GDEY029T94 e-paper 2.9" monochrome display
@version 1.0
@date    2022-09-20
@author  Aram Vartanyan, based on the il3820 driver by Juergen Kienhoefer
 */

/*********************
 *      INCLUDES
 *********************/
#include "disp_spi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ssd1680.h"

/*********************
 *      DEFINES
 *********************/
 #define TAG "SSD1680"

/**
 * ssd1680 compatible EPD controller driver.
 */

#define BIT_SET(a,b)                    ((a) |= (1U<<(b)))
#define BIT_CLEAR(a,b)                  ((a) &= ~(1U<<(b)))

/* Number of pixels? */
#define SSD1680_PIXEL                   (LV_HOR_RES_MAX * LV_VER_RES_MAX)

#define EPD_PANEL_NUMOF_COLUMS		EPD_PANEL_WIDTH
#define EPD_PANEL_NUMOF_ROWS_PER_PAGE	8

/* Are pages the number of bytes to represent the panel width? in bytes */
#define EPD_PANEL_NUMOF_PAGES	        (EPD_PANEL_HEIGHT / EPD_PANEL_NUMOF_ROWS_PER_PAGE)

#define SSD1680_PANEL_FIRST_PAGE	        0
#define SSD1680_PANEL_LAST_PAGE	        (EPD_PANEL_NUMOF_PAGES - 1)
#define SSD1680_PANEL_FIRST_GATE	        0
#define SSD1680_PANEL_LAST_GATE	        (EPD_PANEL_NUMOF_COLUMS - 1)

#define SSD1680_PIXELS_PER_BYTE		    8
#define EPD_PARTIAL_CNT                 5

//uint8_t ssd1680_scan_mode = SSD1680_DATA_ENTRY_XIYDX; //another approach
uint8_t ssd1680_scan_mode = SSD1680_DATA_ENTRY_XIYIY; //as per the il3820 driver

static uint8_t partial_counter = 0;

static uint8_t ssd1680_border_init[] = {0x05}; //init
static uint8_t ssd1680_border_part[] = {0x80}; //partial update
//A2 - 1 Follow LUT; A1:A0 - 01 LUT1;
//static uint8_t ssd1680_border[] = {0x03}; //old LUT3

/* Static functions */

static void ssd1680_update_display(bool isPartial);
static inline void ssd1680_set_window( uint16_t sx, uint16_t ex, uint16_t ys, uint16_t ye);
static inline void ssd1680_set_cursor(uint16_t sx, uint16_t ys);
static inline void ssd1680_waitbusy(int wait_ms);
static inline void ssd1680_hw_reset(void);
static inline void ssd1680_command_mode(void);
static inline void ssd1680_data_mode(void);
static inline void ssd1680_write_cmd(uint8_t cmd, uint8_t *data, size_t len);
static inline void ssd1680_send_cmd(uint8_t cmd);
static inline void ssd1680_send_data(uint8_t *data, uint16_t length);

/* Required by LVGL */
void ssd1680_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    /* Each byte holds the data of 8 pixels, linelen is the number of bytes
     * we need to cover a line of the display. */
    size_t linelen = EPD_PANEL_WIDTH / 8; //SSD1680_COLUMNS = (EPD_PANEL_WIDTH / 8)
    uint8_t *buffer = (uint8_t *) color_map;
    uint16_t x_addr_counter = 0;
    uint16_t y_addr_counter = 0;
    
    /* Set the cursor at the beginning of the graphic RAM */
#if defined (CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT)
    x_addr_counter = EPD_PANEL_WIDTH - 1;
    y_addr_counter = EPD_PANEL_HEIGHT - 1;
#endif
    ssd1680_init();
    
    if (!partial_counter) {
        ESP_LOGD(TAG, "Refreshing in FULL");
        ssd1680_send_cmd(SSD1680_CMD_WRITE1_RAM);
        for(size_t row = 0; row <= (EPD_PANEL_HEIGHT - 1); row++){
            ssd1680_send_data(buffer, linelen);
            buffer += SSD1680_COLUMNS;
        }
        ssd1680_send_cmd(SSD1680_CMD_WRITE2_RAM);
        for(size_t row = 0; row <= (EPD_PANEL_HEIGHT - 1); row++){
            ssd1680_send_data(buffer, linelen);
            buffer += SSD1680_COLUMNS;
        }
        ssd1680_update_display(false);
        partial_counter = EPD_PARTIAL_CNT;
    } else {
        //update partial
        ssd1680_hw_reset();
        ssd1680_write_cmd(SSD1680_CMD_BWF_CTRL, ssd1680_border_part, 1);
        ssd1680_set_window(area->x1, area->x2, area->y1, area->y2);
        ssd1680_set_cursor(x_addr_counter, y_addr_counter);
        
        ssd1680_send_cmd(SSD1680_CMD_WRITE1_RAM);
        for(size_t row = 0; row <= (EPD_PANEL_HEIGHT - 1); row++) {
            ssd1680_send_data(buffer, linelen);
            buffer += SSD1680_COLUMNS; //(128/8)x296 = 4736
        }
        ssd1680_update_display(true);
        partial_counter--;
    }
    ssd1680_deep_sleep();
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(drv);
}

/* Specify the start/end positions of the window address in the X and Y
 * directions by an address unit.
 *
 * @param sx: X Start position.
 * @param ex: X End position.
 * @param ys: Y Start position.
 * @param ye: Y End position.
 */
static inline void ssd1680_set_window( uint16_t sx, uint16_t ex, uint16_t ys, uint16_t ye)
{
    uint8_t tmp[4] = {0};

    tmp[0] = sx / 8;
    tmp[1] = ex / 8; //0x0F-->(15+1)*8=128 -> ex = EPD_PANEL_WIDTH
    //tmp[1] = (ex / 8) - 1;

    /* Set X address start/end */
    ssd1680_write_cmd(SSD1680_CMD_RAM_XPOS_CTRL, tmp, 2);

    //0x0127-->(295+1)=296
    //a % b = a - (a/b)*b
    tmp[0] = ys % 256;
    tmp[1] = ys / 256;
    tmp[2] = ye % 256;
    tmp[3] = ye / 256;
    /* Set Y address start/end */
    ssd1680_write_cmd(SSD1680_CMD_RAM_YPOS_CTRL, tmp, 4);
}

/* Make initial settings for the RAM X and Y addresses in the address counter
 * (AC).
 *
 * @param sx: RAM X address counter.
 * @param ys: RAM Y address counter.
 */
static inline void ssd1680_set_cursor(uint16_t sx, uint16_t ys)
{
    uint8_t tmp[2] = {0};

    tmp[0] = sx / 8;
    ssd1680_write_cmd(SSD1680_CMD_RAM_XPOS_CNTR, tmp, 1);

    tmp[0] = ys % 256;
    tmp[1] = ys / 256;
    ssd1680_write_cmd(SSD1680_CMD_RAM_YPOS_CNTR, tmp, 2);
}

/* After sending the RAM content we need to send the commands:
 * - Display Update Control 2
 * - Master Activation
 */
static void ssd1680_update_display(bool isPartial)
{
    uint8_t tmp = 0;

    if(isPartial) {
    tmp = 0xFF; //Display mode 2 - Partial update
    } else {
    tmp = 0xF7; //Display mode 1 - Full update
    }
    /* Display Update Control */
    ssd1680_write_cmd(SSD1680_CMD_UPDATE_CTRL2, &tmp, 1);
    /* Activate Display Update Sequence */
    ssd1680_write_cmd(SSD1680_CMD_MASTER_ACTIVATION, NULL, 0);
    /* Poll BUSY signal. */
    ssd1680_waitbusy(SSD1680_WAIT);
}

/* Rotate the display by "software" when using PORTRAIT orientation.
 * BIT_SET(byte_index, bit_index) clears the bit_index pixel at byte_index of
 * the display buffer.
 * BIT_CLEAR(byte_index, bit_index) sets the bit_index pixel at the byte_index
 * of the display buffer. */
void ssd1680_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t* buf,
    lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{
    uint16_t byte_index = 0;
    uint8_t  bit_index = 0;
    
#if defined (CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT)
    //This part doesn't work for now. Display prints random dots.
    byte_index = x + ((y >> 3) * EPD_PANEL_HEIGHT);
    bit_index  = y & 0x7;

    if (color.full) {
        BIT_SET(buf[byte_index], 7 - bit_index);
    } else {
        uint16_t mirrored_idx = (EPD_PANEL_HEIGHT - x) + ((y >> 3) * EPD_PANEL_HEIGHT);
        BIT_CLEAR(buf[mirrored_idx], 7 - bit_index);
    }
#elif defined (CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE)
    byte_index = y + ((x >> 3) * EPD_PANEL_HEIGHT);
    bit_index  = x & 0x7;

    if (color.full) {
        BIT_SET(buf[byte_index], 7 - bit_index);
    } else {
        BIT_CLEAR(buf[byte_index], 7 - bit_index);
    }
#else
#error "Unsupported orientation used"
#endif
}

/* Required by LVGL */
void ssd1680_rounder(lv_disp_drv_t * disp_drv, lv_area_t *area)
{
    area->x1 = area->x1 & ~(0x7);
    area->x2 = area->x2 |  (0x7);
    
    /* Update the areas as needed.
     * For example it makes the area to start only on 8th rows and have Nx8 pixel height.*/
}

/* main initialization routine */
void ssd1680_init(void)
{
    uint8_t tmp[3] = {0};

    /* Initialize non-SPI GPIOs */
    gpio_pad_select_gpio(SSD1680_DC_PIN);
    gpio_set_direction(SSD1680_DC_PIN, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(SSD1680_BUSY_PIN);
    gpio_set_direction(SSD1680_BUSY_PIN,  GPIO_MODE_INPUT);

#if SSD1680_USE_RST
    gpio_pad_select_gpio(SSD1680_RST_PIN);
    gpio_set_direction(SSD1680_RST_PIN, GPIO_MODE_OUTPUT);

    /* Harware reset */
    ssd1680_hw_reset();
#endif

    /* Busy wait for the BUSY signal to go low */
    ssd1680_waitbusy(SSD1680_WAIT);
    
    /* Software reset */
    ssd1680_write_cmd(SSD1680_CMD_SW_RESET, NULL, 0);

    ssd1680_waitbusy(SSD1680_WAIT);

    /* Driver output control */
    tmp[0] = (EPD_PANEL_HEIGHT - 1) & 0xFF; //0x27
    tmp[1] = (EPD_PANEL_HEIGHT >> 8 );      //0x01
    tmp[2] = 0x00; // GD = 0; SM = 0; TB = 0;  //0x00
    ssd1680_write_cmd(SSD1680_CMD_GDO_CTRL, tmp, 3);

    /* Configure entry mode  */
    ssd1680_write_cmd(SSD1680_CMD_ENTRY_MODE, &ssd1680_scan_mode, 1);
    
    /* Configure the window */
    ssd1680_set_window(0, EPD_PANEL_WIDTH - 1, 0, EPD_PANEL_HEIGHT - 1);
    
    /* Select border waveform for VBD */
    ssd1680_write_cmd(SSD1680_CMD_BWF_CTRL, ssd1680_border_init, 1);
    
    /* Display update control 1 */
    tmp[0] = 0x00; //A7:0 Normal RAM content option
    tmp[1] = 0x80; //B7 Source Output Mode: Available source from S8 to S167
    tmp[2] = 0x00; //do not send
    ssd1680_write_cmd(SSD1680_CMD_UPDATE_CTRL1, tmp, 2);
    
    /* Read Build-in Temperature sensor */
    tmp[0] = 0x80; //A7:0 Internal sensor
    tmp[1] = 0x00; //do not send
    ssd1680_write_cmd(SSD1680_CMD_READ_INT_TEMP, tmp, 1);
    
    /*set RAM x (0) and y (295)  address count */
    ssd1680_set_cursor(0, EPD_PANEL_HEIGHT - 1);
    ssd1680_waitbusy(SSD1680_WAIT);
}

/* Enter deep sleep mode */
void ssd1680_deep_sleep(void)
{
    uint8_t data[] = {0x01};

    /* Wait for the BUSY signal to go low */
    ssd1680_waitbusy(SSD1680_WAIT);

    ssd1680_write_cmd(SSD1680_CMD_SLEEP_MODE1, data, 1);
    vTaskDelay(100 / portTICK_RATE_MS); // 100ms delay
}

static inline void ssd1680_waitbusy(int wait_ms)
{
    int i = 0;

    vTaskDelay(10 / portTICK_RATE_MS); // 10ms delay

    for(i = 0; i < (wait_ms * 10); i++) {
        if(gpio_get_level(SSD1680_BUSY_PIN) != SSD1680_BUSY_LEVEL) {
            return;
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    ESP_LOGE( TAG, "busy exceeded %dms", i*10 );
}

/* Set HWRESET */
static inline void ssd1680_hw_reset(void)
{
    gpio_set_level(SSD1680_RST_PIN, 0);
    vTaskDelay(SSD1680_RESET_DELAY / portTICK_RATE_MS);
    gpio_set_level(SSD1680_RST_PIN, 1);
    vTaskDelay(SSD1680_RESET_DELAY / portTICK_RATE_MS);
}

/* Set DC signal to command mode */
static inline void ssd1680_command_mode(void)
{
    gpio_set_level(SSD1680_DC_PIN, 0);
}

/* Set DC signal to data mode */
static inline void ssd1680_data_mode(void)
{
    gpio_set_level(SSD1680_DC_PIN, 1);
}

static inline void ssd1680_write_cmd(uint8_t cmd, uint8_t *data, size_t len)
{
    disp_wait_for_pending_transactions();
    ssd1680_command_mode();
    disp_spi_send_data(&cmd, 1);

    if (data != NULL) {
	ssd1680_data_mode();
	disp_spi_send_data(data, len);
    }
}

/* Send cmd to the display */
static inline void ssd1680_send_cmd(uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    ssd1680_command_mode();
    disp_spi_send_data(&cmd, 1);
}

/* Send length bytes of data to the display */
static inline void ssd1680_send_data(uint8_t *data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    ssd1680_data_mode();
    disp_spi_send_colors(data, length);
}

