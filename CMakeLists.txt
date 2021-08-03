if(ESP_PLATFORM)

file(GLOB SOURCES *.c)
set(LVGL_INCLUDE_DIRS . lvgl_tft)
list(APPEND SOURCES "lvgl_tft/disp_driver.c")

#@todo add SimleInclude macro here

# Build all display drivers
    list(APPEND SOURCES "lvgl_tft/ili9341.c")
    list(APPEND SOURCES "lvgl_tft/ili9481.c")
    list(APPEND SOURCES "lvgl_tft/ili9486.c")
    list(APPEND SOURCES "lvgl_tft/ili9488.c")
    list(APPEND SOURCES "lvgl_tft/st7789.c")
    list(APPEND SOURCES "lvgl_tft/st7735s.c")
    list(APPEND SOURCES "lvgl_tft/st7796s.c")
    list(APPEND SOURCES "lvgl_tft/hx8357.c")
    list(APPEND SOURCES "lvgl_tft/sh1107.c")
    list(APPEND SOURCES "lvgl_tft/ssd1306.c")
    list(APPEND SOURCES "lvgl_tft/EVE_commands.c")
    list(APPEND SOURCES "lvgl_tft/FT81x.c")
    list(APPEND SOURCES "lvgl_tft/il3820.c")
    list(APPEND SOURCES "lvgl_tft/jd79653a.c")
    list(APPEND SOURCES "lvgl_tft/uc8151d.c")
    list(APPEND SOURCES "lvgl_tft/ra8875.c")
    list(APPEND SOURCES "lvgl_tft/GC9A01.c")
    list(APPEND SOURCES "lvgl_tft/ili9163c.c")

if(CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI)
    list(APPEND SOURCES "lvgl_tft/disp_spi.c")
endif()

# Add touch driver to compilation only if it is selected in menuconfig
if(CONFIG_LV_TOUCH_CONTROLLER)
    list(APPEND SOURCES "lvgl_touch/touch_driver.c")
    list(APPEND LVGL_INCLUDE_DIRS lvgl_touch)

    # Include only the source file of the selected
    # touch controller.
    if(CONFIG_LV_TOUCH_CONTROLLER_XPT2046)
        list(APPEND SOURCES "lvgl_touch/xpt2046.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_FT6X06)
        list(APPEND SOURCES "lvgl_touch/ft6x36.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_STMPE610)
        list(APPEND SOURCES "lvgl_touch/stmpe610.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_ADCRAW)
        list(APPEND SOURCES "lvgl_touch/adcraw.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_FT81X)
        list(APPEND SOURCES "lvgl_touch/FT81x.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_RA8875)
        list(APPEND SOURCES "lvgl_touch/ra8875_touch.c")
    elseif(CONFIG_LV_TOUCH_CONTROLLER_GT911)
        list(APPEND SOURCES "lvgl_touch/gt911.c")
    endif()

    if(CONFIG_LV_TOUCH_DRIVER_PROTOCOL_SPI)
        list(APPEND SOURCES "lvgl_touch/tp_spi.c")
    endif()
endif()

# Add backlight control to compilation only if it is selected in menuconfig
if(CONFIG_LV_ENABLE_BACKLIGHT_CONTROL)
    list(APPEND SOURCES "lvgl_tft/esp_lcd_backlight.c")
endif()

if(CONFIG_LV_I2C)
    list(APPEND SOURCES "lvgl_i2c/i2c_manager.c")
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${LVGL_INCLUDE_DIRS}
                       REQUIRES lvgl)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_LVGL_H_INCLUDE_SIMPLE")

else()
    message(FATAL_ERROR "LVGL ESP32 drivers: ESP_PLATFORM is not defined. Try reinstalling ESP-IDF.")
endif()
