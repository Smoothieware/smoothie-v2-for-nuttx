To build the test cases do ```rake testing=1```
then flash to Bambino, the results print out to the uart/serial

To make the Firmware do ```rake```

Firmware currently runs on UART at 115300 baud and on USB serial at ttyACM0.
On USB Serial you need to hit Enter to get it to start.
A formatted sdcard must be inserted for it to start, although no files are required to be on the sdcard at this time.
The config file will be config.ini an example is shown here... 
https://gist.github.com/8996097730ba1f17413e3aad1c98eaaf
