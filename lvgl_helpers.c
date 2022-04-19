/**
 * @file lvgl_helpers.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl_helpers.h"

#include "sdkconfig.h"

#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_idf_version.h"

#include "disp_spi.h"
#include "tp_spi.h"

#include "lvgl_spi_conf.h"

#include "lvgl_i2c/i2c_manager.h"

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

 #define TAG "lvgl_helpers"

#define GPIO_NOT_USED   (-1)
#define DMA_DEFAULT_TRANSFER_SIZE   (0u)
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * Calculates the SPI max transfer size based on the display buffer size
 *
 * @return SPI max transfer size in bytes
 */
static int calculate_spi_max_transfer_size(const int display_buffer_size);

#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)
/**
 * Handle FT81X initialization as it's a particular case
 */
static void init_ft81x(int dma_channel);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/* Interface (SPI, I2C) initialization */
void lvgl_interface_init(void)
{
    /* Since LVGL v8 LV_HOR_RES_MAX and LV_VER_RES_MAX are not defined, so
     * print it only if they are defined. */
#if (LVGL_VERSION_MAJOR < 8)
    ESP_LOGI(TAG, "Display hor size: %d, ver size: %d", LV_HOR_RES_MAX, LV_VER_RES_MAX);
#endif

    size_t display_buffer_size = lvgl_get_display_buffer_size();

    ESP_LOGI(TAG, "Display buffer size: %d", display_buffer_size);

#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)
    init_ft81x(dma_channel);
    return;
#endif

/* Display controller initialization */
#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI) || defined (SHARED_SPI_BUS)
    ESP_LOGI(TAG, "Initializing SPI master");

    int miso = DISP_SPI_MISO;
    int spi_max_transfer_size = calculate_spi_max_transfer_size(display_buffer_size);

    /* Set the miso signal to be the selected for the touch driver */
#if defined (SHARED_SPI_BUS)
    miso = TP_SPI_MISO;
#endif

    // We use DMA channel 1 for all cases
    lvgl_spi_driver_init(TFT_SPI_HOST, miso, DISP_SPI_MOSI, DISP_SPI_CLK,
        spi_max_transfer_size, 1, DISP_SPI_IO2, DISP_SPI_IO3);

    disp_spi_add_device(TFT_SPI_HOST);

    /* Add device for touch driver */
#if defined (SHARED_SPI_BUS)
    tp_spi_add_device(TOUCH_SPI_HOST);
    touch_driver_init();

    return;
#endif

#elif defined (CONFIG_LV_I2C_DISPLAY)
#else
#error "No protocol defined for display controller"
#endif

/* Touch controller initialization */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
#if defined (CONFIG_LV_TOUCH_DRIVER_PROTOCOL_SPI)
    ESP_LOGI(TAG, "Initializing SPI master for touch");

#if defined (CONFIG_IDF_TARGET_ESP32)
    dma_channel = 2;
#endif

    lvgl_spi_driver_init(TOUCH_SPI_HOST, TP_SPI_MISO, TP_SPI_MOSI, TP_SPI_CLK,
        DMA_DEFAULT_TRANSFER_SIZE, dma_channel, GPIO_NOT_USED, GPIO_NOT_USED);

    tp_spi_add_device(TOUCH_SPI_HOST);

    touch_driver_init();
#elif defined (CONFIG_LV_I2C_TOUCH)
    touch_driver_init();
#elif defined (CONFIG_LV_TOUCH_DRIVER_ADC)
    touch_driver_init();
#elif defined (CONFIG_LV_TOUCH_DRIVER_DISPLAY)
    touch_driver_init();
#else
#error "No protocol defined for touch controller"
#endif
#else
#endif
}

void lvgl_display_gpios_init(void)
{
    esp_err_t err = ESP_OK;
    gpio_config_t io_conf = {
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

#ifdef CONFIG_LV_DISPLAY_USE_DC
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_LV_DISP_PIN_DC);
    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);
#endif

#ifdef CONFIG_LV_DISP_USE_RST
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_LV_DISP_PIN_RST);
    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);
#endif

#if !defined(CONFIG_LV_DISP_BACKLIGHT_OFF) && defined(CONFIG_LV_DISP_PIN_BCKL) && \
    (CONFIG_LV_DISP_PIN_BCKL > 0)
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_LV_DISP_PIN_BCKL);
    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);
#endif

#ifdef CONFIG_LV_DISP_USE_BUSY
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_LV_DISP_PIN_BUSY);
    err = gpio_config(&io_conf);
    ESP_ERROR_CHECK(err);
#endif
}

/* DISP_BUF_SIZE value doesn't have an special meaning, but it's the size
 * of the buffer(s) passed to LVGL as display buffers. The default values used
 * were the values working for the contributor of the display controller.
 *
 * As LVGL supports partial display updates the DISP_BUF_SIZE doesn't
 * necessarily need to be equal to the display size.
 *
 * When using RGB displays the display buffer size will also depends on the
 * color format being used, for RGB565 each pixel needs 2 bytes.
 * When using the mono theme, the display pixels can be represented in one bit,
 * so the buffer size can be divided by 8, e.g. see SSD1306 display size. */
size_t lvgl_get_display_buffer_size(void)
{
    size_t disp_buffer_size = 0;

#if LVGL_VERSION_MAJOR < 8
#if defined (CONFIG_CUSTOM_DISPLAY_BUFFER_SIZE)
    disp_buffer_size = CONFIG_CUSTOM_DISPLAY_BUFFER_BYTES;
#else
    /* Calculate total of 40 lines of display horizontal size */
#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7789)   ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7796S)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_HX8357)   ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9481)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9486)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9488)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9341)  ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)    ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_RA8875)   ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_GC9A01)   ||  \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9163C)
    disp_buffer_size = LV_HOR_RES_MAX * 40;
#elif defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SH1107
    disp_buffer_size = LV_HOR_RES_MAX * LV_VER_RES_MAX;
#elif defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306
#if defined (CONFIG_LV_THEME_MONO)
    disp_buffer_size = LV_HOR_RES_MAX * (LV_VER_RES_MAX / 8);
#else
    disp_buffer_size = LV_HOR_RES_MAX * LV_VER_RES_MAX);
#endif
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820)
    disp_buffer_size = LV_VER_RES_MAX * IL3820_COLUMNS;
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A)
    disp_buffer_size = ((LV_VER_RES_MAX * LV_VER_RES_MAX) / 8); // 5KB
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D)
    disp_buffer_size = ((LV_VER_RES_MAX * LV_VER_RES_MAX) / 8); // 2888 bytes
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_PCD8544)
    disp_buffer_size = (LV_HOR_RES_MAX * (LV_VER_RES_MAX / 8));
#else
#error "No display controller selected"
#endif
#endif

#else /* LVGL v8 */
    /* ToDo: Implement display buffer size calculation with configuration values from the display driver */
    disp_buffer_size = 320*40; // Reasonable for start
#endif

    return disp_buffer_size;
}

/* Initialize spi bus master
 *
 * NOTE: dma_chan type and value changed to int instead of spi_dma_chan_t
 * for backwards compatibility with ESP-IDF versions prior v4.3.
 *
 * We could use the ESP_IDF_VERSION_VAL macro available in the "esp_idf_version.h"
 * header available since ESP-IDF v4.
 */
bool lvgl_spi_driver_init(int host,
    int miso_pin, int mosi_pin, int sclk_pin,
    int max_transfer_sz,
    int dma_channel,
    int quadwp_pin, int quadhd_pin)
{
    const char *spi_names[] = {
        "SPI1_HOST", "SPI2_HOST", "SPI3_HOST"
    };

    ESP_LOGI(TAG, "Configuring SPI host %s", spi_names[host]);
    ESP_LOGI(TAG, "MISO pin: %d, MOSI pin: %d, SCLK pin: %d, IO2/WP pin: %d, IO3/HD pin: %d",
        miso_pin, mosi_pin, sclk_pin, quadwp_pin, quadhd_pin);

    ESP_LOGI(TAG, "Max transfer size: %d (bytes)", max_transfer_sz);

    spi_bus_config_t buscfg = {
        .miso_io_num = miso_pin,
        .mosi_io_num = mosi_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = quadwp_pin,
        .quadhd_io_num = quadhd_pin,
        .max_transfer_sz = max_transfer_sz
    };

    ESP_LOGI(TAG, "Initializing SPI bus...");
    esp_err_t ret = spi_bus_initialize((spi_host_device_t) host, &buscfg, dma_channel);
    assert(ret == ESP_OK);

    return ESP_OK != ret;
}

static int calculate_spi_max_transfer_size(const int display_buffer_size)
{
    int retval = 0;

#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9481) || \
    defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9488)
    retval = display_buffer_size * 3;
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9341)  || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7789)   || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S)  || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_HX8357)   || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_SH1107)   || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)    || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820)   || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A) || \
      defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9163C)
    retval = display_buffer_size * 2;
#else
    retval = display_buffer_size * 2;
#endif

    return retval;
}

#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)
static void init_ft81x(int dma_channel)
{
    size_t display_buffer_size = lvgl_get_display_buffer_size();
    int spi_max_transfer_size = calculate_spi_max_transfer_size(display_buffer_size);

    lvgl_spi_driver_init(TFT_SPI_HOST, DISP_SPI_MISO, DISP_SPI_MOSI, DISP_SPI_CLK,
        spi_max_transfer_size, dma_channel, DISP_SPI_IO2, DISP_SPI_IO3);

    disp_spi_add_device(TFT_SPI_HOST);

#if defined (CONFIG_LV_TOUCH_CONTROLLER_FT81X)
    touch_driver_init();
#endif
}
#endif
