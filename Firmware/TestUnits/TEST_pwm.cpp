#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>

#include "TestRegistry.h"

#include "lpc_types.h"
#include "chip-defs.h"
#include "sct_18xx_43xx.h"
#include "sct_pwm_18xx_43xx.h"
#include "scu_18xx_43xx.h"


#define SCT_PWM            LPC_SCT

#define SCT_PWM_PIN_OUT    4        /* COUT4 Generate square wave */
#define SCT_PWM_PIN_LED    5        /* COUT5 [index 2] Controls LED */

#define SCT_PWM_OUT        1        /* Index of OUT PWM */
#define SCT_PWM_LED        2        /* Index of LED PWM */
#define SCT_PWM_RATE   10000        /* PWM frequency 10 KHz */


REGISTER_TEST(PWMTest, basic)
{
    /* Initialize the SCT as PWM and set frequency */
    Chip_SCTPWM_Init(SCT_PWM);
    Chip_SCTPWM_SetRate(SCT_PWM, SCT_PWM_RATE);

    /* SCT_OUT5 on P2.11 mapped to FUNC1: LED2 */
    Chip_SCU_PinMuxSet(0x2, 11, (SCU_MODE_INACT | SCU_MODE_FUNC1));
    /* SCT_OUT4 on P2.12 mapped to FUNC1: Oscilloscope input */
    Chip_SCU_PinMuxSet(0x2, 12, (SCU_MODE_INACT | SCU_MODE_FUNC1));

    /* Use SCT0_OUT1 pin */
    Chip_SCTPWM_SetOutPin(SCT_PWM, SCT_PWM_OUT, SCT_PWM_PIN_OUT);
    Chip_SCTPWM_SetOutPin(SCT_PWM, SCT_PWM_LED, SCT_PWM_PIN_LED);

    /* Start with 50% duty cycle */
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PWM_OUT, Chip_SCTPWM_PercentageToTicks(SCT_PWM, 50));
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PWM_LED, Chip_SCTPWM_PercentageToTicks(SCT_PWM, 25));
    Chip_SCTPWM_Start(SCT_PWM);
}

#if 1
#include <string>
#include <cstring>
#include <cctype>

#include <tuple>
#include <vector>
/* 43xx Pinmap for PWM to CTOUT and function
Pin  a, b, COUT#, Function
*/
static std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> lut {
    {0x01, 1,  7,  1},
    {0x01, 2,  6,  1},
    {0x01, 3,  8,  1},
    {0x01, 4,  9,  1},
    {0x01, 5,  10, 1},
    {0x01, 7,  13, 2},
    {0x01, 8,  12, 2},
    {0x01, 9,  11, 2},
    {0x01, 10, 14, 2},
    {0x01, 11, 15, 2},
    {0x02, 7,  1,  1},
    {0x02, 8,  0,  1},
    {0x02, 9,  3,  1},
    {0x02, 10, 2,  1},
    {0x02, 11, 5,  1},
    {0x02, 12, 4,  1},
    {0x04, 1,  1,  1},
    {0x04, 2,  0,  1},
    {0x04, 3,  3,  1},
    {0x04, 4,  2,  1},
    {0x04, 5,  5,  1},
    {0x04, 6,  4,  1},
    {0x06, 5,  6,  1},
    {0x06, 12, 7,  1},
    {0x07, 0,  14, 1},
    {0x07, 1,  15, 1},
    {0x07, 4,  13, 1},
    {0x07, 5,  12, 1},
    {0x07, 6,  11, 1},
    {0x07, 7,  8,  1},
    {0x0A, 4,  9,  1},
    {0x0B, 0,  10, 1},
    {0x0B, 1,  6,  5},
    {0x0B, 2,  7,  5},
    {0x0B, 3,  8,  5},
    {0x0D, 0,  15, 1},
    {0x0D, 2,  7,  1},
    {0x0D, 3,  6,  1},
    {0x0D, 4,  8,  1},
    {0x0D, 5,  9,  1},
    {0x0D, 6,  10, 1},
    {0x0D, 9,  13, 1},
    {0x0D, 11, 14, 6},
    {0x0D, 12, 10, 6},
    {0x0D, 13, 13, 6},
    {0x0D, 14, 11, 6},
    {0x0D, 15, 8,  6},
    {0x0D, 16, 12, 6},
    {0x0E, 5,  3,  1},
    {0x0E, 6,  2,  1},
    {0x0E, 7,  5,  1},
    {0x0E, 8,  4,  1},
    {0x0E, 11, 12, 1},
    {0x0E, 12, 11, 1},
    {0x0E, 13, 14, 1},
    {0x0E, 15, 0,  1},
    {0x0F, 9,  1,  2}
};

static bool lookup_pin(uint8_t port, uint8_t pin, uint8_t& ctout, uint8_t& func)
{
    for(auto &p : lut) {
        if(port == std::get<0>(p) && pin == std::get<1>(p)) {
            ctout= std::get<2>(p);
            func= std::get<3>(p);
            return true;
        }
    }

    return false;
}

static int pwm_index= 1;
static int map_pin_to_pwm(const char *name)
{
    // specify pin name P1.6 and check it is mappable to a PWM channel
    if(tolower(name[0]) == 'p') {
        // pin specification
        std::string str(name);
        uint16_t port = strtol(str.substr(1).c_str(), nullptr, 16);
        size_t pos = str.find_first_of("._", 1);
        if(pos == std::string::npos) return 0;
        uint16_t pin = strtol(str.substr(pos + 1).c_str(), nullptr, 10);

        // now map to a PWM output
        uint8_t ctout, func;
        if(!lookup_pin(port, pin, ctout, func)) {
            return 0;
        }

        // check if ctoun is already in use
        // TODO

        // setup pin for the PWM function
        Chip_SCU_PinMuxSet(port, pin, func);

        // TODO index is incremented for each pin
        Chip_SCTPWM_SetOutPin(SCT_PWM, pwm_index, ctout);

        return pwm_index++;
    }

    return 0;
}

REGISTER_TEST(PWMTest, map_pin)
{
    uint8_t ctout, func;
    TEST_ASSERT_TRUE(lookup_pin(2, 11, ctout, func));
    TEST_ASSERT_EQUAL_INT(ctout, 5);
    TEST_ASSERT_EQUAL_INT(func, 1);

    TEST_ASSERT_FALSE(lookup_pin(2, 13, ctout, func));
    TEST_ASSERT_EQUAL_INT(map_pin_to_pwm("X1.2"), 0);
    TEST_ASSERT_EQUAL_INT(map_pin_to_pwm("P2.12"), 1);
}


// current = dutycycle * 2.0625
REGISTER_TEST(PWMTest, set_current)
{
    // set X driver to 400mA
    // set Y driver to 1amp
    // set Z driver to 1.5amp
    int xind= map_pin_to_pwm("P7.4"); // X
    TEST_ASSERT_TRUE(xind > 0);
    // dutycycle= current/2.0625
    // TODO we need to calculate ticks ourselves as uint8 percentage is not very accurate
    uint8_t dcp= floorf((0.4F*100)/2.0625F);
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, xind, Chip_SCTPWM_PercentageToTicks(SCT_PWM, dcp));

    int yind= map_pin_to_pwm("PB.2"); // Y
    TEST_ASSERT_TRUE(yind > xind);

    // dutycycle= current/2.0625
    dcp= floorf((1.0F*100)/2.0625F);
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, yind, Chip_SCTPWM_PercentageToTicks(SCT_PWM, dcp));

    int zind= map_pin_to_pwm("PB.3"); // Z
    TEST_ASSERT_TRUE(zind > yind);
    // dutycycle= current/2.0625
    dcp= floorf((1.5F*100)/2.0625F);
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, zind, Chip_SCTPWM_PercentageToTicks(SCT_PWM, dcp));

}
#endif
