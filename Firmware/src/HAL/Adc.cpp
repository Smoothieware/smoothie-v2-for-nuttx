// Uses the lpcopen ADC driver instead of the NuttX one whioch doessn't really do what we want.

#include "Adc.h"
//#include "Median.h"

#include "lpc_types.h"
#include "adc_18xx_43xx.h"
#include "chip-defs.h"

#include <cstring>
#include <algorithm>

#include "nuttx/arch.h"

#define _LPC_ADC_ID LPC_ADC0
const ADC_CHANNEL_T CHANNEL_LUT[] = {
    ADC_CH0,                /**< ADC channel 0 */
    ADC_CH1,                /**< ADC channel 1 */
    ADC_CH2,                /**< ADC channel 2 */
    ADC_CH3,                /**< ADC channel 3 */
    ADC_CH4,                /**< ADC channel 4 */
    ADC_CH5,                /**< ADC channel 5 */
    ADC_CH6,                /**< ADC channel 6 */
    ADC_CH7                /**< ADC channel 7 */
};

Adc *Adc::instances[Adc::num_channels] = {nullptr};
int Adc::ninstances = 0;

static ADC_CLOCK_SETUP_T ADCSetup;

// warning we cannot create these once ADC is running
Adc::Adc()
{
    instance_idx = ninstances++;
    instances[instance_idx] = this;
    channel = -1;
    enabled = false;
}

Adc::~Adc()
{
    Chip_ADC_EnableChannel(_LPC_ADC_ID, CHANNEL_LUT[channel], DISABLE);
    channel = -1;
    enabled = false;
    // remove from instances array
    irqstate_t flags = enter_critical_section();
    instances[instance_idx] = nullptr;
    for (int i = instance_idx; i < ninstances - 1; ++i) {
        instances[i] = instances[i + 1];
    }
    --ninstances;
    leave_critical_section(flags);
}

bool Adc::setup()
{
    // ADC Init
    Chip_ADC_Init(_LPC_ADC_ID, &ADCSetup);

    // ADC sample rate need to be fast enough to be able to read the enabled channels within the thermistor poll time
    // even though there maybe 32 samples we only need one new one within the polling time
    // Set sample rate to 1KHz
    Chip_ADC_SetSampleRate(_LPC_ADC_ID, &ADCSetup, 1000);

    // Select using burst mode
    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, ENABLE);

    // init instances array
    for (int i = 0; i < num_channels; ++i) {
        instances[i] = nullptr;
    }
    return true;
}

bool Adc::start()
{
    // setup to interrupt
    int ret = irq_attach(LPC43M4_IRQ_ADC0, Adc::sample_isr, NULL);
    if (ret == OK) {
        up_enable_irq(LPC43M4_IRQ_ADC0);

    } else {
        return false;
    }

    return true;
}

bool Adc::stop()
{
    up_disable_irq(LPC43M4_IRQ_ADC0);
    irq_attach(LPC43M4_IRQ_ADC0, nullptr, nullptr);
    Chip_ADC_SetBurstCmd(_LPC_ADC_ID, DISABLE);
    Chip_ADC_DeInit(_LPC_ADC_ID);

    return true;
}

// TODO only ADC0_0 to ADC0_7 handled at the moment
// figure out channel from name (ADC0_1, ADC0_4, ...)
Adc* Adc::from_string(const char *name)
{
    if(enabled) return nullptr; // aready setup

    const char *p = strcasestr(name, "adc");
    if (p == nullptr) return nullptr;
    if(*p++ != '0') return nullptr; // must be ADC0
    if(*p++ != '_') return nullptr; // must be _
    channel = strtol(p, nullptr, 10);
    if(channel < 0 || channel >= num_channels) return nullptr;

    memset(sample_buffer, 0, sizeof(sample_buffer));
    memset(ave_buf, 0, sizeof(ave_buf));
    Chip_ADC_EnableChannel(_LPC_ADC_ID, CHANNEL_LUT[channel], ENABLE);
    enabled = true;

    return this;
}

// NUTTX isr call
int Adc::sample_isr(int irq, void *context, FAR void *arg)
{
    for (int i = 0; i < ninstances; ++i) {
        Adc *adc = Adc::getInstance(i);
        if(adc == nullptr || !adc->enabled) continue; // not setup

        int ch = adc->channel;
        if(ch < 0) continue; // no channel assigned
        uint16_t dataADC = 0;
        if(Chip_ADC_ReadStatus(_LPC_ADC_ID, CHANNEL_LUT[ch], ADC_DR_DONE_STAT) == SET && Chip_ADC_ReadValue(_LPC_ADC_ID, CHANNEL_LUT[ch], &dataADC) == SUCCESS) {
            adc->new_sample(dataADC);
        }
    }

    return OK;
}

// Keeps the last 8 values for each channel
// This is called in an ISR, so sample_buffer needs to be accessed atomically
void Adc::new_sample(uint32_t value)
{
    // Shuffle down and add new value to the end
    memmove(&sample_buffer[0], &sample_buffer[1], sizeof(sample_buffer) - sizeof(sample_buffer[0]));
    sample_buffer[num_samples - 1] = value; // the 12 bit ADC reading
}

//#define USE_MEDIAN_FILTER
// Read the filtered value ( burst mode ) on a given pin
uint32_t Adc::read()
{
    uint16_t median_buffer[num_samples];

    // needs atomic access TODO maybe be able to use std::atomic here or some lockless mutex
    irqstate_t flags = enter_critical_section();
    memcpy(median_buffer, sample_buffer, sizeof(median_buffer));
    leave_critical_section(flags);

#ifdef USE_MEDIAN_FILTER
    // returns the median value of the last 8 samples
    return median_buffer[quick_median(median_buffer, num_samples)];

#elif defined(OVERSAMPLE)
    // Oversample to get 2 extra bits of resolution
    // weed out top and bottom worst values then oversample the rest
    std::sort(median_buffer, median_buffer + num_samples);
    uint32_t sum = 0;
    for (int i = num_samples / 4; i < (num_samples - (num_samples / 4)); ++i) {
        sum += median_buffer[i];
    }

    // put into a 4 element moving average and return the average of the last 4 oversampled readings
    // this slows down the rate of change a little bit
    ave_buf[3] = ave_buf[2];
    ave_buf[2] = ave_buf[1];
    ave_buf[1] = ave_buf[0];
    ave_buf[0] = sum >> OVERSAMPLE;
    return roundf((ave_buf[0] + ave_buf[1] + ave_buf[2] + ave_buf[3]) / 4.0F);

#else
    // sort the 8 readings and return the average of the middle 4
    std::sort(median_buffer, median_buffer + num_samples);
    int sum = 0;
    for (int i = num_samples / 4; i < (num_samples - (num_samples / 4)); ++i) {
        sum += median_buffer[i];
    }
    return sum / (num_samples / 2);

#endif
}
