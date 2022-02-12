/**
 * @file st7789.c
 *
 * Mostly taken from lbthomsen/esp-idf-littlevgl github.
 */

#include "st7789.h"

#include "disp_spi.h"
#include "display_port.h"

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
static void st7789_set_orientation(lv_disp_drv_t *drv, uint8_t orientation);
static void st7789_send_cmd(lv_disp_drv_t * drv, uint8_t cmd);
static void st7789_send_data(lv_disp_drv_t * drv, void *data, uint16_t length);
static void st7789_send_color(lv_disp_drv_t * drv, void *data, uint16_t length);
static void st7789_reset(lv_disp_drv_t * drv);

static void setup_initial_offsets(lv_disp_drv_t * drv);
/**********************
 *  STATIC VARIABLES
 **********************/
static uint16_t user_x_offset = 0u;
static uint16_t user_y_offset = 0u;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void st7789_init(lv_disp_drv_t *drv)
{
    setup_initial_offsets(drv);

    lcd_init_cmd_t st7789_init_cmds[] = {
        {0xCF, {0x00, 0x83, 0X30}, 3},
        {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
        {ST7789_PWCTRL2, {0x85, 0x01, 0x79}, 3},
        {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
        {0xF7, {0x20}, 1},
        {0xEA, {0x00, 0x00}, 2},
        {ST7789_LCMCTRL, {0x26}, 1},
        {ST7789_IDSET, {0x11}, 1},
        {ST7789_VCMOFSET, {0x35, 0x3E}, 2},
        {ST7789_CABCCTRL, {0xBE}, 1},
        {ST7789_MADCTL, {0x00}, 1}, // Set to 0x28 if your display is flipped
        {ST7789_COLMOD, {0x55}, 1},

#if ST7789_INVERT_COLORS == 1
		{ST7789_INVON, {0}, 0}, // set inverted mode
#else
 		{ST7789_INVOFF, {0}, 0}, // set non-inverted mode
#endif

        {ST7789_RGBCTRL, {0x00, 0x1B}, 2},
        {0xF2, {0x08}, 1},
        {ST7789_GAMSET, {0x01}, 1},
        {ST7789_PVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17}, 14},
        {ST7789_NVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
        {ST7789_CASET, {0x00, 0x00, 0x00, 0xEF}, 4},
        {ST7789_RASET, {0x00, 0x00, 0x01, 0x3f}, 4},
        {ST7789_RAMWR, {0}, 0},
        {ST7789_GCTRL, {0x07}, 1},
        {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
        {ST7789_SLPOUT, {0}, 0x80},
        {ST7789_DISPON, {0}, 0x80},
        {0, {0}, 0xff},
    };

    st7789_reset(drv);

    //Send all the commands
    uint16_t cmd = 0;
    while (st7789_init_cmds[cmd].databytes!=0xff) {
        st7789_send_cmd(drv, st7789_init_cmds[cmd].cmd);
        st7789_send_data(drv, st7789_init_cmds[cmd].data, st7789_init_cmds[cmd].databytes&0x1F);
        if (st7789_init_cmds[cmd].databytes & 0x80) {
            display_port_delay(drv, 100);
        }
        cmd++;
    }

    /* NOTE: Setting rotation from lv_disp_drv_t instead of menuconfig */
    st7789_set_orientation(drv, lv_disp_get_rotation((lv_disp_t *) drv));
}

/* The ST7789 display controller can drive up to 320*240 displays, when using a 240*240 or 240*135
 * displays there's a gap of 80px or 40/52/53px respectively. 52px or 53x offset depends on display orientation.
 * We need to edit the coordinates to take into account those gaps, this is not necessary in all orientations. */
void st7789_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    uint8_t data[4] = {0};

    uint16_t offsetx1 = area->x1;
    uint16_t offsetx2 = area->x2;
    uint16_t offsety1 = area->y1;
    uint16_t offsety2 = area->y2;
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    offsetx1 += st7789_x_offset();
    offsetx2 += st7789_x_offset();
    offsety1 += st7789_y_offset();
    offsety2 += st7789_y_offset();

    /*Column addresses*/
    st7789_send_cmd(drv, ST7789_CASET);
    data[0] = (offsetx1 >> 8) & 0xFF;
    data[1] = offsetx1 & 0xFF;
    data[2] = (offsetx2 >> 8) & 0xFF;
    data[3] = offsetx2 & 0xFF;
    st7789_send_data(drv, data, 4);

    /*Page addresses*/
    st7789_send_cmd(drv, ST7789_RASET);
    data[0] = (offsety1 >> 8) & 0xFF;
    data[1] = offsety1 & 0xFF;
    data[2] = (offsety2 >> 8) & 0xFF;
    data[3] = offsety2 & 0xFF;
    st7789_send_data(drv, data, 4);

    /*Memory write*/
    st7789_send_cmd(drv, ST7789_RAMWR);
    st7789_send_color(drv, (void*) color_map, size * 2);
}

void st7789_set_x_offset(const uint16_t offset)
{
    user_x_offset = offset;
}

void st7789_set_y_offset(const uint16_t offset)
{
    user_y_offset = offset;
}

uint16_t st7789_x_offset(void)
{
    return user_x_offset;
}

uint16_t st7789_y_offset(void)
{
    return user_y_offset;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void st7789_send_cmd(lv_disp_drv_t *drv, uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    display_port_gpio_dc(drv, 0);
    disp_spi_send_data(&cmd, 1);
}

static void st7789_send_data(lv_disp_drv_t *drv, void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    display_port_gpio_dc(drv, 1);
    disp_spi_send_data(data, length);
}

static void st7789_send_color(lv_disp_drv_t *drv, void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    display_port_gpio_dc(drv, 1);
    disp_spi_send_colors(data, length);
}

/* Reset the display, if we don't have a reset pin we use software reset */
static void st7789_reset(lv_disp_drv_t *drv)
{
#if !defined(ST7789_SOFT_RST)
    display_port_gpio_rst(drv, 0);
    display_port_delay(drv, 100);
    display_port_gpio_rst(drv, 1);
    display_port_delay(drv, 100);
#else
    st7789_send_cmd(drv, ST7789_SWRESET);
    display_port_delay(drv, 5);
#endif
}

static void st7789_set_orientation(lv_disp_drv_t *drv, uint8_t orientation)
{
    uint8_t data[] =
    {
#if CONFIG_LV_PREDEFINED_DISPLAY_TTGO
	0x60, 0xA0, 0x00, 0xC0
#else
	0xC0, 0x60, 0x00, 0xA0
#endif
    };

    st7789_send_cmd(drv, ST7789_MADCTL);
    st7789_send_data(drv, (void *) &data[orientation], 1);
}

static void setup_initial_offsets(lv_disp_drv_t * drv)
{
    lv_disp_rot_t rotation = lv_disp_get_rotation((lv_disp_t *) drv);

#if (CONFIG_LV_TFT_DISPLAY_OFFSETS)
    st7789_set_x_offset(CONFIG_LV_TFT_DISPLAY_X_OFFSET);
    st7789_set_y_offset(CONFIG_LV_TFT_DISPLAY_Y_OFFSET);

#elif (LV_HOR_RES_MAX == 240) && (LV_VER_RES_MAX == 240)
    if (LV_DISP_ROT_NONE == rotation)
    {
        st7789_set_x_offset(80);
        st7789_set_y_offset(0);
    }
    else if (LV_DISP_ROT_270 == rotation)
    {
        st7789_set_x_offset(0);
        st7789_set_y_offset(80);
    }
#elif (LV_HOR_RES_MAX == 240) && (LV_VER_RES_MAX == 135)
    if (LV_DISP_ROT_NONE == rotation || LV_DISP_ROT_180 == rotation)
    {
        st7789_set_x_offset(40);
        st7789_set_y_offset(53);
    }
#elif (LV_HOR_RES_MAX == 135) && (LV_VER_RES_MAX == 240)
    if (LV_DISP_ROT_90 == rotation || LV_DISP_ROT_270 == rotation)
    {
        st7789_set_x_offset(52);
        st7789_set_y_offset(40);
    }
#endif
}

/* Display update callback, we could update the orientation in here
 * NOTE Available only for LVGL v8 */
void st7789_update_cb(lv_disp_drv_t *drv)
{
    (void) drv;
}
