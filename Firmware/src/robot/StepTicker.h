#pragma once

#include <stdint.h>
#include <array>
#include <bitset>
#include <functional>
#include <atomic>

#include "ActuatorCoordinates.h"

class StepperMotor;
class Block;

// handle 2.62 Fixed point
#define STEP_TICKER_FREQUENCY 100000.0F
#define STEPTICKER_FPSCALE (1LL<<62)
#define STEPTICKER_FROMFP(x) ((float)(x)/STEPTICKER_FPSCALE)

class StepTicker
{
public:
    StepTicker();
    ~StepTicker();
    void set_frequency( float frequency );
    void set_unstep_time( float microseconds );
    int register_actuator(StepperMotor* motor);
    float get_frequency() const { return frequency; }
    void unstep_tick();
    const Block *get_current_block() const { return current_block; }

    void step_tick (void);
    void handle_finish (void);
    bool start();
    bool stop();

    // whatever setup the block should register this to know when it is done
    std::function<void()> finished_fnc{nullptr};

    static StepTicker *getInstance() { return instance; }

private:
    static StepTicker *instance;
    bool start_unstep_ticker();
    int initial_setup(const char *dev, void *timer_handler);
    bool start_next_block();
    std::array<StepperMotor*, k_max_actuators> motor;
    std::bitset<k_max_actuators> unstep;

    Block *current_block{nullptr};
    float frequency{0};
    uint32_t period{0};
    uint32_t current_tick{0};

    int step_fd{-1};
    int unstep_fd{-1};

    uint8_t num_motors{0};

    volatile bool running{false};
    bool started{false};
};
