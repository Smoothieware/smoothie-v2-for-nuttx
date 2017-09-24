To build the test cases do ```rake testing=1```
then flash to Bambino, the results print out to the uart/serial

To make the Firmware do ```rake target=Bambino```

Firmware currently runs on UART at 115200 baud and on USB serial at ttyACM0.
On USB Serial you need to hit Enter to get it to start.

The config file will be config.ini an example is shown here...
https://gist.github.com/8996097730ba1f17413e3aad1c98eaaf

Currently the config.ini is builtin and defined in string-config-bambino.h

Currently Robot is ported and should allow XYZ motors to work as expected at 100Khz max step rate.
Also enough is ported to run a 3D printer, Hotends, extruders etc

NOTE for the smooothiev2 mini alpha replace Bambino above with Minialpha...

```rake target=Minialpha```

config is in string-config-minialpha.h

The Mini Alpha is not currently working very well.

Make sure you follow the ../README.md file for creating the nuttx-export.Bambino (or nuttx-export.Minialpha)
