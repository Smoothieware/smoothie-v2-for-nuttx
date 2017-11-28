To build the test cases do ```rake testing=1```
then flash to Bambino, the results print out to the uart/serial

To make the Firmware do ```rake target=Bambino```

Firmware currently runs on UART at 115200 baud and on USB serial at ttyACM0.
On USB Serial you need to hit Enter to get it to start.

The config file is called config.ini on the sdcard and examples are shown in the ConfigSamples diretory, config-3d.ini is for a 3d printer, and config-laser.ini is for laser, these would be renamed config.ini and copied to the sdcard.

The config.ini may also be builtin and is defined in string-config-bambino.h, a #define is needed in the main.cpp to use the builtin config.ini.

Currently the max stepping rate is limited to 100Khz as this seems the upper limit to handle the 10us interrupt.

Enough modules have been ported to run a 3D printer, also a laser is supported.

Modules that have been ported so far...

* endstops
* extruder
* laser
* switch
* temperaturecontrol
* zprobe
* currentcontrol
* killbutton

*NOTE* for the smooothiev2 mini alpha replace Bambino above with Minialpha...

```rake target=Minialpha```

builtin config would be called string-config-minialpha.h (but the default is to read the config.ini on sdcard).

The Mini Alpha is currently working quite well.

Make sure you follow the ../README.md file for creating the nuttx-export.Bambino (or nuttx-export.Minialpha)

on the Mini Alpha there are 4 leds..

led1 - nuttx system led, on when nuttx is running ok
led2 - nuttx syste led, flashes when there is interrupt activity, hard on when there is a crash.
led3 - smoothie led, flashes slowly when idle, does not flash when busy
led4 - smoothie led, TBD

