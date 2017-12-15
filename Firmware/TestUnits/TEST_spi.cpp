#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <map>
#include <set>
#include <istream>

#include "TestRegistry.h"

#include "Spi.h"
#include "Pin.h"

#include "lpc_types.h"
#include "chip-defs.h"
#include "spi_18xx_43xx.h"
#include "ssp_18xx_43xx.h"
#include "scu_18xx_43xx.h"

/* Read and print temperature data from max31855 sensor */
REGISTER_TEST(SPITest, max31855_write)
{
    //configure and enable SSP1
    Spi *spi=new Spi(2);
    TEST_ASSERT_TRUE(spi->from_string(2,"p1_3",Spi::MISO));
    TEST_ASSERT_TRUE(spi->from_string(2,"pF_4",Spi::SCLK));
    Pin *spi_cs_pin=new Pin();
    TEST_ASSERT_TRUE(spi_cs_pin->from_string("p1_5"));
    spi_cs_pin->set(true);
    spi_cs_pin->as_output();

    //read data n times (default: 1000 tests)
    int n_tests=1000;
    for(int i=0;i<n_tests;i++) {
        spi_cs_pin->set(false);
        //  Read first 16 bits
        uint16_t data = spi->write(0);
        //  Read last 16 bits
        // uint16_t data2 = spi->write(0);
        spi_cs_pin->set(true);
        data = data >> 2;
        float temperature = (data & 0x1FFF) / 4.f;
        if (data & 0x2000) {
            data = ~data;
            temperature = ((data & 0x1FFF) + 1) / -4.f;
        }
        printf("temperature = %0.1f ºC\n",temperature);
    }
}
