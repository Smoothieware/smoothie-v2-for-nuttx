#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>

#include "TestRegistry.h"

#include "lpc_types.h"
#include "adc_18xx_43xx.h"
#include "chip-defs.h"

#define _ADC_CHANNEL ADC_CH3
#define _LPC_ADC_ID LPC_ADC0
#define _LPC_ADC_IRQ ADC0_IRQn

static ADC_CLOCK_SETUP_T ADCSetup;

REGISTER_TEST(ADCTest, polling)
{
    /*ADC Init */
    Chip_ADC_Init(_LPC_ADC_ID, &ADCSetup);
    Chip_ADC_EnableChannel(_LPC_ADC_ID, _ADC_CHANNEL, ENABLE);

    uint16_t dataADC;

    /* Select using burst mode or not */
    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, ENABLE);

    for (int i = 0; i < 100; ++i) {
        /* Start A/D conversion if not using burst mode */
        //    Chip_ADC_SetStartMode(_LPC_ADC_ID, ADC_START_NOW, ADC_TRIGGERMODE_RISING);

        /* Waiting for A/D conversion complete */
        while (Chip_ADC_ReadStatus(_LPC_ADC_ID, _ADC_CHANNEL, ADC_DR_DONE_STAT) != SET) {}
        /* Read ADC value */
        Chip_ADC_ReadValue(_LPC_ADC_ID, _ADC_CHANNEL, &dataADC);
        /* Print ADC value */
        printf("adc= %04X\n", dataADC);
        usleep(100000);
    }

    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, DISABLE);
}
