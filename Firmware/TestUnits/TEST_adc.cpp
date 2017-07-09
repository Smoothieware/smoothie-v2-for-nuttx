#include "../Unity/src/unity.h"
#include <stdlib.h>
#include <stdio.h>

#include "TestRegistry.h"

#include "lpc_types.h"
#include "adc_18xx_43xx.h"
#include "chip-defs.h"

#include "Adc.h"

#define _ADC_CHANNEL ADC_CH3
#define _LPC_ADC_ID LPC_ADC0
#define _LPC_ADC_IRQ ADC0_IRQn


REGISTER_TEST(ADCTest, polling)
{
    ADC_CLOCK_SETUP_T ADCSetup;
    /*ADC Init */
    Chip_ADC_Init(_LPC_ADC_ID, &ADCSetup);
    Chip_ADC_EnableChannel(_LPC_ADC_ID, _ADC_CHANNEL, ENABLE);

    uint16_t dataADC;

    // Set sample rate to 1KHz
    Chip_ADC_SetSampleRate(_LPC_ADC_ID, &ADCSetup, 1000);

    /* Select using burst mode or not */
    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, ENABLE);

    for (int i = 0; i < 100; ++i) {
        /* Start A/D conversion if not using burst mode */
        //    Chip_ADC_SetStartMode(_LPC_ADC_ID, ADC_START_NOW, ADC_TRIGGERMODE_RISING);

        /* Waiting for A/D conversion complete */
        while (Chip_ADC_ReadStatus(_LPC_ADC_ID, _ADC_CHANNEL, ADC_DR_DONE_STAT) != SET) {}
        /* Read ADC value */
        if(Chip_ADC_ReadValue(_LPC_ADC_ID, _ADC_CHANNEL, &dataADC) == SUCCESS) {
            printf("adc= %04X\n", dataADC);
        } else {
            printf("Failed to read adc\n");
        }
        usleep(100000);
    }

    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, DISABLE);
    Chip_ADC_EnableChannel(_LPC_ADC_ID, _ADC_CHANNEL, DISABLE);
    Chip_ADC_DeInit(_LPC_ADC_ID);
}

REGISTER_TEST(ADCTest, Adc_class_interrupts)
{
    TEST_ASSERT_TRUE(Adc::setup());

    Adc *adc = new Adc;
    TEST_ASSERT_FALSE(adc->from_string("nc") == adc);
    TEST_ASSERT_TRUE(adc->from_string("ADC0_3") == adc);

    TEST_ASSERT_TRUE(Adc::start());

    for (int i = 0; i < 10; ++i) {
        uint16_t v= adc->read();
        printf("adc= %04X\n", v);
        usleep(50000);
    }

    delete adc;

    TEST_ASSERT_TRUE(Adc::stop());

}
