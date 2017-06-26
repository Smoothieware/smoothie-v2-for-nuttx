#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include "TestRegistry.h"

#include "LinearDeltaSolution.h"
#include "ActuatorCoordinates.h"

#include "ConfigReader.h"
static std::string str("[linear delta]\n");

REGISTER_TEST(TimeTest, delta_ik)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);

    float millimeters[3]= {100.0, 200.0, 300.0};
    ActuatorCoordinates ac;
    BaseSolution* k= new LinearDeltaSolution(cr);

    uint32_t n= 10000000;
    systime_t st = clock_systimer();

    for(uint32_t i=0;i<n;i++) k->cartesian_to_actuator( millimeters, ac);

    systime_t en = clock_systimer();
    printf("elapsed time %d us over %d iterations %1.4f us per iteration\n", TICK2USEC(en-st), n, TICK2USEC(en-st)/(float)n);

    delete k;

    TEST_PASS();
}
