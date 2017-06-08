# smoothie-v2
Smoothie for version 2 boards using nuttx as base, using https://github.com/Smoothieware/smoothie-nuttx

Dev/... is for devlopment and testing
Firmware/... is for Smoothie firmware code

First you need to clone and build the smoothie-nuttx with "make export".
Then unzip the nuttx-export.zip in Firmware/.

To build ```cd Firmware; rake```

To compile only some unit tests:

rake testing=1 test=streams

rake testing=1 test=dispatch

To compile with debug symbols:

rake testing=1 test=streams debug=1

You need to install ruby (and rake)
