#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string>

extern "C" {
    void lpc43_gpio_write(uint16_t gpiocfg, bool value);
    bool lpc43_gpio_read(uint16_t gpiocfg);
}

class Pin
{
public:
    Pin();
    Pin(const char *s);

    Pin* from_string(std::string value);

    inline bool connected()
    {
        return this->valid;
    }

    Pin* as_output();
    Pin* as_input();

    inline bool get()
    {
        if (!this->valid) return false;
        return this->inverting ^ lpc43_gpio_read(gpiocfg);
    }

    inline void set(bool value)
    {
        if (!this->valid) return;
        if ( this->inverting ^ value ) {
            lpc43_gpio_write(gpiocfg, true);
        } else {
            lpc43_gpio_write(gpiocfg, false);
        }
    }

    uint16_t get_gpiocfg() const { return gpiocfg; }

    // mbed::PwmOut *hardware_pwm();

    // mbed::InterruptIn *interrupt_pin();

    // bool is_inverting() const { return inverting; }
    // void set_inverting(bool f) { inverting = f; }

private:
    //Added pinName
    uint16_t gpiocfg;

    struct {
        bool inverting: 1;
        bool valid: 1;
        bool adc_only: 1;   //true if adc only pin
        int adc_channel: 8;   //adc channel
    };
};
