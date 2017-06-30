#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <cmath>

#include "TestRegistry.h"

#include "LinearDeltaSolution.h"
#include "ActuatorCoordinates.h"

#include "ConfigReader.h"

__attribute__  ((section (".ramfunctions"))) void runMemoryTest()
{
    register uint32_t* p = (uint32_t *)0x14000000;
    register uint32_t r1;
    register uint32_t r2;
    register uint32_t r3;
    register uint32_t r4;
    register uint32_t r5;
    register uint32_t r6;
    register uint32_t r7;
    register uint32_t r8;

    uint32_t n= 8000000;
    systime_t st = clock_systimer();
    while(p < (uint32_t *)(0x14000000+n)) {
        asm volatile ("ldm.w %[ptr]!,{%[r1],%[r2],%[r3],%[r4],%[r5],%[r6],%[r7],%[r8]}" :
                        [r1] "=r" (r1), [r2] "=r" (r2), [r3] "=r" (r3), [r4] "=r" (r4),
                        [r5] "=r" (r5), [r6] "=r" (r6),[r7] "=r" (r7), [r8] "=r" (r8),
                        [ptr] "=r" (p)                                                  :
                        "r" (p)                                                         : );
    }
    systime_t en = clock_systimer();

    printf("elapsed time %d us over %d bytes %1.4f mb/sec\n", TICK2USEC(en-st), n, (float)n/TICK2USEC(en-st));
}

void configureSPIFI();
float get_pll1_clk();

REGISTER_TEST(TimeTest, read_flash)
{
    get_pll1_clk();

    configureSPIFI();

    get_pll1_clk();

    runMemoryTest();
}

static std::string str("[linear delta]\n");
REGISTER_TEST(TimeTest, delta_ik)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);

    float millimeters[3]= {100.0, 200.0, 300.0};
    ActuatorCoordinates ac;
    BaseSolution* k= new LinearDeltaSolution(cr);

    uint32_t n= 100000;
    systime_t st = clock_systimer();

    for(uint32_t i=0;i<n;i++) k->cartesian_to_actuator( millimeters, ac);

    systime_t en = clock_systimer();
    printf("elapsed time %d us over %d iterations %1.4f us per iteration\n", TICK2USEC(en-st), n, TICK2USEC(en-st)/(float)n);

    delete k;

    TEST_PASS();
}

// REGISTER_TEST(TimeTest, isnan)
// {
//     float f= NAN;
//     TEST_ASSERT_TRUE(::isnan(f));
//     TEST_ASSERT_FALSE(::isnan(0.0F));
// }
