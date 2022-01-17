/**
 * @file ili9225.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "ili9225.h"
#include "disp_spi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ILI9225"
#define ILI9225_DRIVER_OUTPUT_CTRL 0x01      // Driver Output Control
#define ILI9225_LCD_AC_DRIVING_CTRL 0x02     // LCD AC Driving Control
#define ILI9225_ENTRY_MODE 0x03              // Entry Mode
#define ILI9225_DISP_CTRL1 0x07              // Display Control 1
#define ILI9225_BLANK_PERIOD_CTRL1 0x08      // Blank Period Control
#define ILI9225_FRAME_CYCLE_CTRL 0x0B        // Frame Cycle Control
#define ILI9225_INTERFACE_CTRL 0x0C          // Interface Control
#define ILI9225_OSC_CTRL 0x0F                // Osc Control
#define ILI9225_POWER_CTRL1 0x10             // Power Control 1
#define ILI9225_POWER_CTRL2 0x11             // Power Control 2
#define ILI9225_POWER_CTRL3 0x12             // Power Control 3
#define ILI9225_POWER_CTRL4 0x13             // Power Control 4
#define ILI9225_POWER_CTRL5 0x14             // Power Control 5
#define ILI9225_VCI_RECYCLING 0x15           // VCI Recycling
#define ILI9225_RAM_ADDR_SET1 0x20           // Horizontal GRAM Address Set
#define ILI9225_RAM_ADDR_SET2 0x21           // Vertical GRAM Address Set
#define ILI9225_GRAM_DATA_REG 0x22           // GRAM Data Register
#define ILI9225_GATE_SCAN_CTRL 0x30          // Gate Scan Control Register
#define ILI9225_VERTICAL_SCROLL_CTRL1 0x31   // Vertical Scroll Control 1 Register
#define ILI9225_VERTICAL_SCROLL_CTRL2 0x32   // Vertical Scroll Control 2 Register
#define ILI9225_VERTICAL_SCROLL_CTRL3 0x33   // Vertical Scroll Control 3 Register
#define ILI9225_PARTIAL_DRIVING_POS1 0x34    // Partial Driving Position 1 Register
#define ILI9225_PARTIAL_DRIVING_POS2 0x35    // Partial Driving Position 2 Register
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 0x36 // Horizontal Address Start Position
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 0x37 // Horizontal Address End Position
#define ILI9225_VERTICAL_WINDOW_ADDR1 0x38   // Vertical Address Start Position
#define ILI9225_VERTICAL_WINDOW_ADDR2 0x39   // Vertical Address End Position
#define ILI9225_GAMMA_CTRL1 0x50             // Gamma Control 1
#define ILI9225_GAMMA_CTRL2 0x51             // Gamma Control 2
#define ILI9225_GAMMA_CTRL3 0x52             // Gamma Control 3
#define ILI9225_GAMMA_CTRL4 0x53             // Gamma Control 4
#define ILI9225_GAMMA_CTRL5 0x54             // Gamma Control 5
#define ILI9225_GAMMA_CTRL6 0x55             // Gamma Control 6
#define ILI9225_GAMMA_CTRL7 0x56             // Gamma Control 7
#define ILI9225_GAMMA_CTRL8 0x57             // Gamma Control 8
#define ILI9225_GAMMA_CTRL9 0x58             // Gamma Control 9
#define ILI9225_GAMMA_CTRL10 0x59            // Gamma Control 10

/**********************
 *      TYPEDEFS
 **********************/

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void ili9225_send_cmd(uint8_t cmd);
static void ili9225_send_cmds(void *data, uint16_t length);
static void ili9225_send_data(void *data, uint16_t length);
static void ili9225_send_color(void *data, uint16_t length);
static void ili9225_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ili9225_init(void)
{
    lcd_init_cmd_t ili_init_cmds[] = {
        {ILI9225_POWER_CTRL1, {0x00, 0x00}, 2},        // Set SAP,DSTB,STB
        {ILI9225_POWER_CTRL2, {0x00, 0x00}, 2},        // Set APON,PON,AON,VCI1EN,VC
        {ILI9225_POWER_CTRL3, {0x00, 0x00}, 2},        // Set BT,DC1,DC2,DC3
        {ILI9225_POWER_CTRL4, {0x00, 0x00}, 2},        // Set GVDD
        {ILI9225_POWER_CTRL5, {0x00, 0x00}, 2 | 0x80}, // Set VCOMH/VCOML voltage, 40ms delay

        {ILI9225_POWER_CTRL2, {0x00, 0x18}, 0x00},     // Set APON,PON,AON,VCI1EN,VC
        {ILI9225_POWER_CTRL3, {0x61, 0x21}, 0x00},     // Set BT,DC1,DC2,DC3
        {ILI9225_POWER_CTRL4, {0x00, 0x6F}, 0x00},     // Set GVDD   007F 0088
        {ILI9225_POWER_CTRL5, {0x49, 0x5F}, 0x00},     // Set VCOMH/VCOML voltage
        {ILI9225_POWER_CTRL1, {0x08, 0x00}, 2 | 0x80}, // Set SAP,DSTB,STB, 10ms delay

        {ILI9225_POWER_CTRL2, {0x10, 0x3B}, 2 | 0x80}, // Set APON,PON,AON,VCI1EN,VC, 50ms delay

        {ILI9225_DRIVER_OUTPUT_CTRL, {0x01, 0x1C}, 2},
        {ILI9225_LCD_AC_DRIVING_CTRL, {0x01, 0x00}, 2},
        {ILI9225_ENTRY_MODE, {0x10, 0x30}, 2},
        {ILI9225_DISP_CTRL1, {0x00, 0x00}, 2},
        {ILI9225_BLANK_PERIOD_CTRL1, {0x08, 0x08}, 2},
        {ILI9225_FRAME_CYCLE_CTRL, {0x11, 0x00}, 2},
        {ILI9225_INTERFACE_CTRL, {0x00, 0x00}, 2},
        {ILI9225_OSC_CTRL, {0x0D, 0x01}, 2},
        {ILI9225_VCI_RECYCLING, {0x00, 0x20}, 2},
        {ILI9225_RAM_ADDR_SET1, {0x00, 0x00}, 2},
        {ILI9225_RAM_ADDR_SET2, {0x00, 0x00}, 2},

        {ILI9225_GATE_SCAN_CTRL, {0x00, 0x00}, 2},

        {ILI9225_GAMMA_CTRL1, {0x00, 0x00}, 2},
        {ILI9225_GAMMA_CTRL2, {0x08, 0x08}, 2},
        {ILI9225_GAMMA_CTRL3, {0x08, 0x0A}, 2},
        {ILI9225_GAMMA_CTRL4, {0x00, 0x0A}, 2},
        {ILI9225_GAMMA_CTRL5, {0x0A, 0x08}, 2},
        {ILI9225_GAMMA_CTRL6, {0x08, 0x08}, 2},
        {ILI9225_GAMMA_CTRL7, {0x00, 0x00}, 2},
        {ILI9225_GAMMA_CTRL8, {0x0A, 0x00}, 2},
        {ILI9225_GAMMA_CTRL9, {0x07, 0x10}, 2},
        {ILI9225_GAMMA_CTRL10, {0x07, 0x10}, 2},

        {ILI9225_DISP_CTRL1, {0x00, 0x12}, 2 | 0x80},
        {ILI9225_DISP_CTRL1, {0x00, 0x17}, 2},
        {0xFF, {0x00}, 0xFF}};

    //Initialize non-SPI GPIOs
    gpio_pad_select_gpio(ILI9225_DC);
    gpio_set_direction(ILI9225_DC, GPIO_MODE_OUTPUT);

#if ILI9225_USE_RST
    gpio_pad_select_gpio(ILI9225_RST);
    gpio_set_direction(ILI9225_RST, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level(ILI9225_RST, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(ILI9225_RST, 0);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(ILI9225_RST, 1);
    vTaskDelay(150 / portTICK_RATE_MS);
#endif

    ESP_LOGI(TAG, "Initialization.");

    //Send all the commands
    uint16_t cmd = 0;
    while (ili_init_cmds[cmd].databytes != 0xFF)
    {
        ili9225_send_cmd(ili_init_cmds[cmd].cmd);
        ili9225_send_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
        if (ili_init_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
}

void ili9225_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    ili9225_set_window(x1, y1, x2, y2);

    uint8_t data[2];
    ili9225_send_cmd(ILI9225_RAM_ADDR_SET1);
    data[0] = (x1 >> 8) & 0xFF;
    data[1] = x1 & 0xFF;
    ili9225_send_data(data, 2);

    ili9225_send_cmd(ILI9225_RAM_ADDR_SET2);
    data[0] = (y1 >> 8) & 0xFF;
    data[1] = y1 & 0xFF;
    ili9225_send_data(data, 2);

    ili9225_send_cmd(ILI9225_GRAM_DATA_REG);

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
    ili9225_send_color((void *)color_map, size * 2);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void ili9225_send_cmd(uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = cmd;

    gpio_set_level(ILI9225_DC, 0); /*Command mode*/
    disp_spi_send_data(data, 2);
}

static void ili9225_send_data(void *data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(ILI9225_DC, 1); /*Data mode*/
    disp_spi_send_data(data, length);
}

static void ili9225_send_color(void *data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(ILI9225_DC, 1); /*Data mode*/
    disp_spi_send_colors(data, length);
}

static void ili9225_set_window(uint16_t h1, uint16_t v1, uint16_t h2, uint16_t v2)
{
    uint8_t data[2];

    ili9225_send_cmd(ILI9225_HORIZONTAL_WINDOW_ADDR1); // HEA
    data[0] = (h2 >> 8) & 0xFF;
    data[1] = h2 & 0xFF;
    ili9225_send_data(data, 2);

    ili9225_send_cmd(ILI9225_HORIZONTAL_WINDOW_ADDR2); // HSA
    data[0] = (h1 >> 8) & 0xFF;
    data[1] = h1 & 0xFF;
    ili9225_send_data(data, 2);

    ili9225_send_cmd(ILI9225_VERTICAL_WINDOW_ADDR1); // VEA
    data[0] = (v2 >> 8) & 0xFF;
    data[1] = v2 & 0xFF;
    ili9225_send_data(data, 2);

    ili9225_send_cmd(ILI9225_VERTICAL_WINDOW_ADDR2); // VSA
    data[0] = (v1 >> 8) & 0xFF;
    data[1] = v1 & 0xFF;
    ili9225_send_data(data, 2);
}
