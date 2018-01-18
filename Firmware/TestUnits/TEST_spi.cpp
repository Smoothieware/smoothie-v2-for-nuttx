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

/* Read and print temperature data from max31855 sensor */
REGISTER_TEST(SPITest, read_max31855)
{
    //open file in Nuttx corresponding to the selected SPI channel
    uint8_t spi_channel=2;
    std::string devpath="/dev/max31855_";
    std::string str_spi_channel=std::to_string(spi_channel);
    devpath+=str_spi_channel;
    int fd = open(devpath.c_str(), O_RDONLY);

    //Configure GPIO Chip select pin
    Max31855::instance[Max31855::instance_index]=new Max31855();
    TEST_ASSERT_TRUE(Max31855::instance[Max31855::instance_index]->spi_cs_pin.from_string("p1_5"));
    Max31855::instance[Max31855::instance_index]->spi_cs_pin.set(true);
    Max31855::instance[Max31855::instance_index]->spi_cs_pin.as_output();

    //read data n times (default: 1000 tests)
    uint16_t data;
    float temperature;
    int n_measurements=1000;
    for(int i=0;i<n_measurements;i++) {
        //Obtain temp value from SPI
        int ret=read(fd, &data, 2);

        //Process temp
        if (ret==-1) {
            //Error
            temperature = std::numeric_limits<float>::infinity();
        } else {
            temperature = (data & 0x1FFF) / 4.f;
        }
        if(!temperature) {
            temperature = std::numeric_limits<float>::infinity();
        }
        printf("temperature = %0.1f ºC\n",temperature);
    }
}
