# smoothie-v2
Smoothie for version 2 boards using nuttx as base, using https://github.com/Smoothieware/smoothie-nuttx

Dev/... is for development and testing
Firmware/... is for Smoothie firmware code and Test Units

Must use the following toolchain..

gcc version 6.3.1 20170215 (release) [ARM/embedded-6-branch revision 245512] (GNU Tools for ARM Embedded Processors 6-2017-q2-update)

(or any 6.x.x will probably work).

First you need to clone and build the smoothie-nuttx with "make export".
Then unzip the nuttx-export.zip in Firmware/nuttx-export.Bambino (or nuttx-export.Minialpha)

documented here https://github.com/Smoothieware/smoothie-nuttx/blob/master/README.md

To build ```cd Firmware; rake target=Bambino```
To build unit tests ```cd Firmware; rake target=Bambino  testing=1```

For Dev ```cd Dev; rake```

To compile only some unit tests in Firmware:

rake target=Bambino testing=1 test=streams

rake target=Bambino testing=1 test=dispatch,streams,planner

To compile with debug symbols:

rake target=Bambino testing=1 test=streams debug=1

You need to install ruby (and rake)

Note for a mini alpha build you need to build the appropriate version of nuttx and export to nuttx-export.Minialpha, and build with ```rake target=Minialpha```
