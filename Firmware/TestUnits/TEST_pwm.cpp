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

/* Systick timer tick rate, to change duty cycle */
#define TICKRATE_HZ     1000        /* 1 ms Tick rate */


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
    Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PWM_LED, 50);
    Chip_SCTPWM_Start(SCT_PWM);
}

