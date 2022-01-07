/**
 * @file ili9488.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "ili9488.h"

#include "disp_spi.h"
#include "display_port.h"
#include "esp_heap_caps.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void ili9488_set_orientation(lv_disp_drv_t * drv, uint8_t orientation);

static void ili9488_send_cmd(lv_disp_drv_t * drv, uint8_t cmd);
static void ili9488_send_data(lv_disp_drv_t * drv, void * data, uint16_t length);
static void ili9488_send_color(lv_disp_drv_t * drv, void * data, uint16_t length);
static void ili9488_reset(lv_disp_drv_t * drv);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
// From github.com/jeremyjh/ESP32_TFT_library
// From github.com/mvturnho/ILI9488-lvgl-ESP32-WROVER-B
void ili9488_init(lv_disp_drv_t * drv)
{
    lcd_init_cmd_t ili_init_cmds[]={
        {ILI9488_CMD_SLEEP_OUT, {0x00}, 0x80},
        {ILI9488_CMD_POSITIVE_GAMMA_CORRECTION, {0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F}, 15},
        {ILI9488_CMD_NEGATIVE_GAMMA_CORRECTION, {0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F}, 15},
        {ILI9488_CMD_POWER_CONTROL_1, {0x17, 0x15}, 2},
        {ILI9488_CMD_POWER_CONTROL_2, {0x41}, 1},
        {ILI9488_CMD_VCOM_CONTROL_1, {0x00, 0x12, 0x80}, 3},
        {ILI9488_CMD_MEMORY_ACCESS_CONTROL, {(0x20 | 0x08)}, 1},
        {ILI9488_CMD_COLMOD_PIXEL_FORMAT_SET, {0x66}, 1},
        {ILI9488_CMD_INTERFACE_MODE_CONTROL, {0x00}, 1},
        {ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL, {0xA0}, 1},
        {ILI9488_CMD_DISPLAY_INVERSION_CONTROL, {0x02}, 1},
        {ILI9488_CMD_DISPLAY_FUNCTION_CONTROL, {0x02, 0x02}, 2},
        {ILI9488_CMD_SET_IMAGE_FUNCTION, {0x00}, 1},
        {ILI9488_CMD_WRITE_CTRL_DISPLAY, {0x28}, 1},
        {ILI9488_CMD_WRITE_DISPLAY_BRIGHTNESS, {0x7F}, 1},
        {ILI9488_CMD_ADJUST_CONTROL_3, {0xA9, 0x51, 0x2C, 0x02}, 4},
        {ILI9488_CMD_DISPLAY_ON, {0x00}, 0x80},
        {0, {0}, 0xff},
    };

    ili9488_reset(drv);

    LV_LOG_INFO("ILI9488 initialization.");

    //Send all the commands
    uint16_t cmd = 0;
    while (ili_init_cmds[cmd].databytes!=0xff) {
        ili9488_send_cmd(drv, ili_init_cmds[cmd].cmd);
        ili9488_send_data(drv, ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes&0x1F);
        
        if (ili_init_cmds[cmd].databytes & 0x80) {
            display_port_delay(drv, 100);
        }

        cmd++;
    }

    ili9488_set_orientation(drv, ILI9488_INITIAL_ORIENTATION);
}

// Flush function based on mvturnho repo
void ili9488_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    lv_color16_t *buffer_16bit = (lv_color16_t *) color_map;
    uint8_t *mybuf;
    do {
        mybuf = (uint8_t *) heap_caps_malloc(3 * size * sizeof(uint8_t), MALLOC_CAP_DMA);
        if (mybuf == NULL) {
            LV_LOG_WARN("Could not allocate enough DMA memory!");
        }
    } while (mybuf == NULL);

    uint32_t LD = 0;
    uint32_t j = 0;

    for (uint32_t i = 0; i < size; i++) {
        LD = buffer_16bit[i].full;
        mybuf[j] = (uint8_t) (((LD & 0xF800) >> 8) | ((LD & 0x8000) >> 13));
        j++;
        mybuf[j] = (uint8_t) ((LD & 0x07E0) >> 3);
        j++;
        mybuf[j] = (uint8_t) (((LD & 0x001F) << 3) | ((LD & 0x0010) >> 2));
        j++;
    }

	/* Column addresses  */
	uint8_t xb[] = {
	    (uint8_t) (area->x1 >> 8) & 0xFF,
	    (uint8_t) (area->x1) & 0xFF,
	    (uint8_t) (area->x2 >> 8) & 0xFF,
	    (uint8_t) (area->x2) & 0xFF,
	};

	/* Page addresses  */
	uint8_t yb[] = {
	    (uint8_t) (area->y1 >> 8) & 0xFF,
	    (uint8_t) (area->y1) & 0xFF,
	    (uint8_t) (area->y2 >> 8) & 0xFF,
	    (uint8_t) (area->y2) & 0xFF,
	};

	/*Column addresses*/
	ili9488_send_cmd(drv, ILI9488_CMD_COLUMN_ADDRESS_SET);
	ili9488_send_data(drv, xb, 4);

	/*Page addresses*/
	ili9488_send_cmd(drv, ILI9488_CMD_PAGE_ADDRESS_SET);
	ili9488_send_data(drv, yb, 4);

	/*Memory write*/
	ili9488_send_cmd(drv, ILI9488_CMD_MEMORY_WRITE);

	ili9488_send_color(drv, (void *) mybuf, size * 3);
	heap_caps_free(mybuf);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline void set_cmd_mode(lv_disp_drv_t * drv)
{
    display_port_gpio_dc(drv, 0);
}

static inline void set_data_mode(lv_disp_drv_t * drv)
{
    display_port_gpio_dc(drv, 1);
}

static void ili9488_send_cmd(lv_disp_drv_t * drv, uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    set_cmd_mode(drv);
    disp_spi_send_data(&cmd, 1);
}

static void ili9488_send_data(lv_disp_drv_t * drv, void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    set_data_mode(drv);
    disp_spi_send_data(data, length);
}

static void ili9488_send_color(lv_disp_drv_t * drv, void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    set_data_mode(drv);
    disp_spi_send_colors(data, length);
}

static void ili9488_set_orientation(lv_disp_drv_t * drv, uint8_t orientation)
{
    assert(orientation < 4);
    const uint8_t data[] = {0x48, 0x88, 0x28, 0xE8};

#if (LV_USE_LOG == 1)
    const char *orientation_str[] = {
        "PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE", "LANDSCAPE_INVERTED"
    };

    LV_LOG_INFO("Display orientation: %s", orientation_str[orientation]);
    LV_LOG_INFO("0x36 command value: 0x%02X", data[orientation]);
#endif

    ili9488_send_cmd(drv, 0x36);
    ili9488_send_data(drv, (void *) &data[orientation], 1);
}

/* Reset the display, if we don't have a reset pin we use software reset */
static void ili9488_reset(lv_disp_drv_t *drv)
{
#if defined(ILI9488_USE_RST)
    display_port_gpio_rst(drv, 0);
    display_port_delay(drv, 100);
    display_port_gpio_rst(drv, 1);
    display_port_delay(drv, 100);
#else
    ili9488_send_cmd(drv, 0x01);
    display_port_delay(drv, 5);
#endif
}
