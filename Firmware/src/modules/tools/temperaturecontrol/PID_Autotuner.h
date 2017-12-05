/**
 * Based on https://github.com/br3ttb/Arduino-PID-AutoTune-Library
 */

#pragma once

#include <stdint.h>

class TemperatureControl;
class GCode;
class OutputStream;

class PID_Autotuner
{
public:
    PID_Autotuner(TemperatureControl *);
    void start(GCode&, OutputStream&);
    void abort();

private:
    void begin(float target, int ncycles);
    void finishUp();

    TemperatureControl *temp_control;
    float target_temperature;

    float *peaks;
    int requested_cycles;
    float noiseBand;
    unsigned long peak1, peak2;
    int sampleTime;
    int nLookBack;
    int lookBackCnt;
    int peakType;
    float *lastInputs;
    int peakCount;
    float absMax, absMin;
    float oStep;
    int output;
    volatile unsigned long tickCnt;
    struct {
        bool justchanged:1;
        volatile bool tick:1;
        bool firstPeak:1;
    };
};
