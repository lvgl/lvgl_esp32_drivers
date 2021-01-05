# lvgl esp32 drivers

# Supported display controllers

**NOTE:** You need to set the display horizontal and vertical size, color depth and
swap of RGB565 color on the LVGL configuration menuconfig (it's not handled automatically).
You can use the rotate member of the `lv_disp_drv_t` type to rotate the screen.

| Display Controller                          | Type       | Interface              | Color depth (LV_COLOR_DEPTH) | Swap the 2 bytes of RGB565 color (LV_COLOR_16_SWAP) |
|---------------------------------------------|------------|------------------------|------------------------------|-----------------------------------------------------|
| ILI9341                                     | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| ILI9486                                     | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| ILI9488                                     | TFT        | SPI                    | 16: RGB565                   | No                                                  |
| HX8357B/HX8357D                             | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| ST7789                                      | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| ST7735S                                     | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| FT81x                                       | TFT        | Single, Dual, Quad SPI | 16: RGB565                   | No                                                  |
| GC9A01                                      | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| RA8875                                      | TFT        | SPI                    | 16: RGB565                   | Yes                                                 |
| SH1107                                      | Monochrome | SPI                    | 1: 1byte per pixel           | No                                                  |
| SSD1306                                     | Monochrome | I2C                    | 1: 1byte per pixel           | No                                                  |
| IL3820                                      | e-Paper    | SPI                    | 1: 1byte per pixel           | No                                                  |
| UC8151D/ GoodDisplay GDEW0154M10 DES        | e-Paper    | SPI                    | 1: 1byte per pixel           | No                                                  |
| FitiPower JD79653A/ GoodDisplay GDEW0154M09 | e-Paper    | SPI                    | 1: 1byte per pixel           | No                                                  |

# Supported indev controllers

- XPT2046
- FT3236
- other FT6X36 or the FT6206 controllers should work as well (not tested)
- STMPE610
- FT81x (Single, Dual, and Quad SPI)

If your display or input device (touch) controller is not supported consider contributing to this repo by
adding support to it! [Contribute controller support](CONTRIBUTE_CONTROLLER_SUPPORT.md)

# Support for predefined development kits

You can also use the predefined kits, which selects the correct display and input device controllers on the kit,
it also sets the pin numbers for the interfaces.

| Kit name                  | Display controller    | Hor. Res. | Ver. Res. | Indev controller  |
|---------------------------|-----------------------|-----------|-----------|-------------------|
| ESP Wrover Kit v4.1       |
| M5Stack                   |
| M5Stick                   |
| M5StickC                  |
| Adafruit 3.5 Featherwing  |
| RPi MPI3501               |
| Wemos Lolin OLED          |
| ER-TFT035-6               |
| AIRcable ATAGv3           |

**NOTE:** See [Supported display controllers](#Supported-display-controllers) for more information on display configuration.
**NOTE:** See [Supported indev controllers](#Supported-indev-controllers) for more information about indev configuration.
