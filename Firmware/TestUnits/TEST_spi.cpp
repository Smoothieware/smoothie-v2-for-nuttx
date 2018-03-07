#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <string>
#include <map>
#include <set>
#include <istream>

#include "TestRegistry.h"

#include "max31855.h"

/* Read and print temperature data from single or multiple max31855 sensor */
REGISTER_TEST(SPITest, read_max31855)
{
 /* open file in Nuttx corresponding to the selected device ID:
        dev_id 0: File /dev/temp0, Channel SSP0, Device 0
        dev_id 1: File /dev/temp1, Channel SSP0, Device 1
        dev_id 2: File /dev/temp2, Channel SSP1, Device 0
        dev_id 3: File /dev/temp3, Channel SSP1, Device 1   */
    
    //Two sensors
    int ninstances=2;
    int fd[ninstances];
    std::string devpath;
    
    //dev_id 2
    devpath = "/dev/temp2";
    fd[0] = open(devpath.c_str(), O_RDONLY);
    
    //dev_id 3
    devpath = "/dev/temp3";
    fd[1] = open(devpath.c_str(), O_RDONLY);    
    
    //read data n times (default: 1000 tests)
    uint16_t data;
    float temperature;
    int ntests=1000;
    int test = 0; 
    while (test < ntests) {
        usleep(100000); // sleep thread during 100 ms
        for(int i = 0; i < ninstances; i++) {
            //Obtain temp value from SPI
             int ret = read(fd[i], &data, 2);

                //Process temp
            if (ret == -1) {
                //Error
                temperature = std::numeric_limits<float>::infinity();
            } else {
                temperature = (data & 0x1FFF) / 4.f;
            }
            printf("instance = %d temperature = %0.1f ºC\n", i, temperature);
            test++;
        }
    }
}
