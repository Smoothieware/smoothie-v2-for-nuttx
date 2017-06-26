#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include "TestRegistry.h"

#include "LinearDeltaSolution.h"
#include "ActuatorCoordinates.h"

#include "ConfigReader.h"
static std::string str("[linear delta]\n");

REGISTER_TEST(TimeTest, delta)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);

    float millimeters[3]= {100.0, 200.0, 300.0};
    ActuatorCoordinates ac;
    BaseSolution* k= new LinearDeltaSolution(cr);

    systime_t st = clock_systimer();

    for(int i=0;i<100;i++) k->cartesian_to_actuator( millimeters, ac);

    systime_t en = clock_systimer();
    printf("elapsed time %d us\n", TICK2USEC(en-st)/100);

    delete k;

    TEST_PASS();
}
