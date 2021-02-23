# TFT drivers 

COMPONENT_SRCDIRS := .
COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_OBJS := disp_driver.o

$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9341),ili9341.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9481),ili9491.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9486),ili9486.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9488),ili9488.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7789),st7789.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S),st7735s.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7796S),st7796s.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_HX8357),hx8357.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_SH1107),sh1107.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306),ssd1306.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X),EVE_commands.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X),FT81x.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820),il3820.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A),jd79653a.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D),uc8151d.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_RA8875),ra8875.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_GC9A01),gc9a01.o)

$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI),disp_spi.o)
