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

REGISTER_TEST(SPITest, from_string)
{
    //Set of examples that initialize SSP0
    Spi *spi;
    TEST_ASSERT_TRUE(spi->from_string(1,"p1.1","miso"));
    TEST_ASSERT_TRUE(spi->from_string(1,"p1.2","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(1,"p3.0","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(1,"p1.0","ssel"));

    //TEST_ASSERT_TRUE(spi->from_string(1,"P9.1","miso"));
    TEST_ASSERT_TRUE(spi->from_string(1,"P9.2","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(1,"P3_0","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(1,"P9.0","ssel"));

    //TEST_ASSERT_TRUE(spi->from_string(1,"pf_2","miso"));
    TEST_ASSERT_TRUE(spi->from_string(1,"pf_3","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(1,"pf_0","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(1,"pf_1","ssel"));

    //Set of examples that initialize SSP1
    //TEST_ASSERT_TRUE(spi->from_string(2,"p0.0","miso"));
    TEST_ASSERT_TRUE(spi->from_string(2,"p0.1","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(2,"p1.19","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(2,"p1.20","ssel"));

    //TEST_ASSERT_TRUE(spi->from_string(2,"pF_6","miso"));
    TEST_ASSERT_TRUE(spi->from_string(2,"pF_7","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(2,"pF_4","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(2,"pF_5","ssel"));

    TEST_ASSERT_TRUE(spi->from_string(2,"p1.3","miso"));
    TEST_ASSERT_TRUE(spi->from_string(2,"p1.4","mosi"));
    TEST_ASSERT_TRUE(spi->from_string(2,"PF_4","sclk"));
    TEST_ASSERT_TRUE(spi->from_string(2,"p1.5","ssel"));

    //Set of examples that fail to initialize SSP0
    TEST_ASSERT_FALSE(spi->from_string(2,"p1.1","miso")); //changed to unsupported channel (2)
    TEST_ASSERT_FALSE(spi->from_string(0,"p1.2","mosi")); //changed channel to unsupported channel (0)
    TEST_ASSERT_FALSE(spi->from_string(1,"p3&0","sclk")); //changed pin name character to p3&0
    TEST_ASSERT_FALSE(spi->from_string(1,"p1.0","sselect")); //changed type name to "sselect"

    TEST_ASSERT_FALSE(spi->from_string(2,"pC_4","miso")); //changed to unsupported pin name
    TEST_ASSERT_FALSE(spi->from_string(2,"pF_7","nc")); //changed to nc (not connected)
    TEST_ASSERT_FALSE(spi->from_string(2,"pF_4","")); //no pin name is set
    TEST_ASSERT_FALSE(spi->from_string(2,"pF_4","ssel")); //changed to unsupported type
}

REGISTER_TEST(SPITest, write)
{
    Spi *spi=new Spi(2);
    //TEST_ASSERT_TRUE(spi->from_string(2,"p1_3","miso"));
    //TEST_ASSERT_TRUE(spi->from_string(2,"pF_4","sclk"));

    Pin *spi_cs_pin=new Pin();
    TEST_ASSERT_TRUE(spi_cs_pin->from_string("p1_5"));
    spi_cs_pin->set(true);
    spi_cs_pin->as_output();

    for(int i=1;i<=1000;i++) {
        spi_cs_pin->set(false);
        uint16_t data = spi->write(0);
        //  Read next 16 bits (diagnostics)
        uint16_t data2 = spi->write(0);
        spi_cs_pin->set(true);
        printf("data=%b %d data2=%b %d\n",data,data,data2,data2);
    }
}

