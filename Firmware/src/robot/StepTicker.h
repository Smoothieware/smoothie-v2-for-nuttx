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
#define STEP_TICKER_FREQUENCY (StepTicker::getInstance()->get_frequency())
#define STEPTICKER_FPSCALE (1LL<<62)
#define STEPTICKER_FROMFP(x) ((float)(x)/STEPTICKER_FPSCALE)

class StepTicker
{
public:
    StepTicker();
    ~StepTicker();
    // delete copy and move constructors and assign operators
    StepTicker(StepTicker const&) = delete;             // Copy construct
    StepTicker(StepTicker&&) = delete;                  // Move construct
    StepTicker& operator=(StepTicker const&) = delete;  // Copy assign
    StepTicker& operator=(StepTicker &&) = delete;      // Move assign

    void set_frequency( float frequency );
    void set_unstep_time( float microseconds );
    int register_actuator(StepperMotor* motor);
    float get_frequency() const { return frequency; }
    const Block *get_current_block() const { return current_block; }

    bool start();

    // whatever setup the block should register this to know when it is done
    std::function<void()> finished_fnc{nullptr};

    static StepTicker *getInstance() { return instance; }
    static uint32_t get_usec_counter() { return usec_counter; }

private:
    static StepTicker *instance;

    void unstep_tick();
    void step_tick (void);
    bool start_unstep_ticker();
    bool start_next_block();

    static void step_timer_handler(void);
    static void unstep_timer_handler(void);
    static void pendsv_handler(void);

    std::array<StepperMotor*, k_max_actuators> motor;

    volatile static uint32_t usec_counter;
    static uint32_t usec_per_tick;

    uint32_t unstep{0}; // one bit set per motor to indicate step pin needs to be unstepped
    uint32_t missed_unsteps{0};

    Block *current_block{nullptr};

    uint32_t frequency{100000}; // Hz
    uint32_t delay{1}; //microseconds

    uint32_t current_tick{0};

    uint8_t num_motors{0};

    volatile bool running{false};
    bool started{false};
};
