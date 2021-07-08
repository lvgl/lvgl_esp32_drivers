# I2C in `lvgl_esp32_drivers`

&nbsp;



## Information for users

### I2C Manager support

`lvgl_esp32_drivers` comes with built-in I2C support by integrating I2C Manager, which is used in case your touch interface or screen uses the I2C bus. The native I2C support offered by ESP-IDF is not thread-safe. Maybe you use LVGL with a touch sensor that has an i2c port, and maybe your device also has another i2c device that needs to be read frequently, such as a 3D-accelerometer. If you read that from another task than the lvgl uses to read the touch data, you need some kind of mechanism to keep these communications from interfering. 

If you have other components that can use I2C Manager (or Mika Tuupola's I2C HAL abstraction that I2C Manager is compatible with) then put I2C Manager in your components directory by cloning the repository from below and in your main program do:

```c
#include "i2c_manager.h"
#include "lvgl_helpers.h"

[...]

lvgl_locking(i2c_manager_locking());
lv_init();
lvgl_driver_init();
```

The `lvgl_locking` part will cause the LVGL I2C driver to play nice with anything else that uses the I2C port(s) through I2C Manager.

See the [I2C Manager GitHub repository](https://github.com/ropg/i2c_manager) for much more information. 


&nbsp;



## Information for driver developers

I2C support in the LVGL ESP drivers is provided exclusively by the files in this directory. Code from all over the project that was talking to the I2C hardware directly has been replaced by code that communicates through the functions provided in `lvgl_i2c.h`. I2C is handled by the I2C Manager that was built into lvlg_esp32_drivers, but the code would be the same if it was routed through I2C Manager as a separate component. If you are providing a driver, you need not worry about any of this.

### Using I2C in a driver, a multi-step guide

#### Step 1

The Kconfig entries for your driver only need to specify that you will be using I2C. This is done by `select LV_I2C_DISPLAY` or `select LV_I2C_TOUCH`.

#### Step 2

To use the I2C port in your code you would do something like:

```c
#include "i2c_manager/i2c_manager.h"

uint8_t data[2];
lvgl_i2c_read(CONFIG_LV_I2C_TOUCH_PORT, 0x23, 0x42, &data, 2);
```

This causes a touch driver to read two bytes at register `0x42` from the IC at address `0x23`. Replace `CONFIG_LV_I2C_TOUCH_PORT` by `CONFIG_LV_I2C_DISPLAY_PORT` when this is a display instead of a touch driver. `lvgl_i2c_write` works much the same way, except writing the bytes from the buffer instead of reading them.

> The example above ignores it but these functions return `esp_err_t` so you can check if the i2c communication worked.

#### Step 3

There is no step 3, you are already done.

### Behind the scenes

If anything in `lvgl_esp32_drivers` uses I2C, the config system will pop up an extra menu. This will allow you to select an I2C port for screen and one for the touch driver, if applicable. An extra menu allows you to set the GPIO pins and bus speed of any port you have selected for use. It's perfectly fine for a display and a touch driver to use the same I2C port or different ones. 


## More information

If you need more documentation, please refer to the [I2C Manager GitHub repository](https://github.com/ropg/i2c_manager) for more detailed information on how I2C manager works.
