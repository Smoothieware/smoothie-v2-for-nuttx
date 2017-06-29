#include "StepTicker.h"

#include "AxisDefns.h"
#include "StepperMotor.h"
#include "Block.h"
#include "Conveyor.h"

#include <nuttx/config.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <nuttx/timers/timer.h>

#include <math.h>

#ifdef STEPTICKER_DEBUG_PIN
// debug pins, only used if defined in src/makefile
#include "Pin.h"
Pin stepticker_debug_pin(STEPTICKER_DEBUG_PIN, Pin::AS_INPUT);
#define SET_STEPTICKER_DEBUG_PIN(n) { stepticker_debug_pin.set(n); }
#else
#define SET_STEPTICKER_DEBUG_PIN(n)
#endif

#define __ramfunc__ __attribute__ ((section(".ramfunctions"),long_call,noinline))

StepTicker *StepTicker::instance;

StepTicker::StepTicker()
{
    instance = this; // setup the Singleton instance of the stepticker
}

StepTicker::~StepTicker()
{
}

// ISR callbacks from timer
__ramfunc__  void step_timer_handler(void)
{
    StepTicker::getInstance()->step_tick();
}

// ISR callbacks from timer
// static void unstep_timer_handler(void)
// {
//     StepTicker::getInstance()->unstep_tick();
// }

extern "C" int highpri_tmr0_setup(uint32_t frequency, void *handler);

bool StepTicker::start()
{
    if(!started) {

        // setup the step timer
        int permod = highpri_tmr0_setup(frequency, (void *)step_timer_handler);
        if(permod <  0) {
            printf("ERROR: tmr0 setup failed\n");
            return false;
        }
        if(permod != 0) {
            printf("Warning: stepticker is not accurate: %d\n", permod);
            // TODO adjust actual step frequency accordingly
        }

        // setup the unstep timer (does not start until needed)
//       unstep_fd = initial_setup(UNSTEP_TICKER_DEVNAME, (void*)unstep_timer_handler, delay);

    }

    started = true;
    current_tick = 0;

    return true;
}

bool StepTicker::stop()
{
    // int ret = ioctl(step_fd, TCIOC_STOP, 0);
    // if (ret < 0) {
    //     printf("ERROR: Failed to stop the step timer: %d\n", errno);
    // }

    // ret = ioctl(unstep_fd, TCIOC_STOP, 0);
    // if (ret < 0) {
    //     printf("ERROR: Failed to stop the unstep timer: %d\n", errno);
    // }

    return false;
}

// Set the base stepping frequency
void StepTicker::set_frequency( float freq )
{
    this->frequency = freq;
    if(freq != this->frequency) {
        this->frequency = freq;
        // if(started) {
        //     stop();

        //     // change frequency of timer callback
        //     int ret = ioctl(step_fd, TCIOC_SETTIMEOUT, period);
        //     if (ret < 0) {
        //         printf("ERROR: Failed to reset the step ticker period to %d: %d\n", period, errno);
        //     }

        //     start();
        // }
    }

}

// Set the reset delay, must be called initial_setup()
void StepTicker::set_unstep_time( float microseconds )
{
    delay = floorf(microseconds);

    // if(started) {
    //     // set frequency of unstep  callback
    //     int ret = ioctl(unstep_fd, TCIOC_SETTIMEOUT, delay);
    //     if (ret < 0) {
    //         printf("ERROR: Failed to set the unstep ticker delay to %d: %d\n", delay, errno);
    //     }
    // }

    // TODO check that the unstep time is less than the step period, if not slow down step ticker
}

__ramfunc__  bool StepTicker::start_unstep_ticker()
{
    // int ret = ioctl(unstep_fd, TCIOC_START, 0);
    // if (ret < 0) {
    //     printf("ERROR: Failed to start the unstep timer: %d\n", errno);
    //     //close(unstep_fd);
    //     //unstep_fd = -1;
    //     return false;
    // }

    // FIXME HACK!! so we can see the pulse on the LA
    unstep_tick();
    return true;
}

// Reset step pins on any motor that was stepped
__ramfunc__  void StepTicker::unstep_tick()
{
    uint32_t bitmsk= 1;
    for (int i = 0; i < num_motors; i++) {
        if(this->unstep & bitmsk) {
            this->motor[i]->unstep();
        }
        bitmsk <<= 1;
    }
    this->unstep= 0;
}

// extern "C" void PendSV_Handler(void)
// {
//     StepTicker::getInstance()->handle_finish();
// }

// slightly lower priority than TIMER0, the whole end of block/start of block is done here allowing the timer to continue ticking
// void StepTicker::handle_finish (void)
// {
//     // all moves finished signal block is finished
//     if(finished_fnc) finished_fnc();
// }

// step clock
__ramfunc__  void StepTicker::step_tick (void)
{
    //SET_STEPTICKER_DEBUG_PIN(running ? 1 : 0);

    // if nothing has been setup we ignore the ticks
    if(!running) {
        // check if anything new available
        if(Conveyor::getInstance()->get_next_block(&current_block)) { // returns false if no new block is available
            running = start_next_block(); // returns true if there is at least one motor with steps to issue
            if(!running) return;
        } else {
            return;
        }
    }

    if(Conveyor::getInstance()->is_halted()) {
        running = false;
        current_tick = 0;
        current_block = nullptr;
        return;
    }

    bool still_moving = false;
    // foreach motor, if it is active see if time to issue a step to that motor
    for (uint8_t m = 0; m < num_motors; m++) {
        if(current_block->tick_info[m].steps_to_move == 0) continue; // not active

        current_block->tick_info[m].steps_per_tick += current_block->tick_info[m].acceleration_change;

        if(current_tick == current_block->tick_info[m].next_accel_event) {
            if(current_tick == current_block->accelerate_until) { // We are done accelerating, deceleration becomes 0 : plateau
                current_block->tick_info[m].acceleration_change = 0;
                if(current_block->decelerate_after < current_block->total_move_ticks) {
                    current_block->tick_info[m].next_accel_event = current_block->decelerate_after;
                    if(current_tick != current_block->decelerate_after) { // We are plateauing
                        // steps/sec / tick frequency to get steps per tick
                        current_block->tick_info[m].steps_per_tick = current_block->tick_info[m].plateau_rate;
                    }
                }
            }

            if(current_tick == current_block->decelerate_after) { // We start decelerating
                current_block->tick_info[m].acceleration_change = current_block->tick_info[m].deceleration_change;
            }
        }

        // protect against rounding errors and such
        if(current_block->tick_info[m].steps_per_tick <= 0) {
            current_block->tick_info[m].counter = STEPTICKER_FPSCALE; // we force completion this step by setting to 1.0
            current_block->tick_info[m].steps_per_tick = 0;
        }

        current_block->tick_info[m].counter += current_block->tick_info[m].steps_per_tick;

        if(current_block->tick_info[m].counter >= STEPTICKER_FPSCALE) { // >= 1.0 step time
            current_block->tick_info[m].counter -= STEPTICKER_FPSCALE; // -= 1.0F;
            ++current_block->tick_info[m].step_count;

            // step the motor
            bool ismoving = motor[m]->step(); // returns false if the moving flag was set to false externally (probes, endstops etc)
            // we stepped so schedule an unstep
            unstep |= (1<<m);

            if(!ismoving || current_block->tick_info[m].step_count == current_block->tick_info[m].steps_to_move) {
                // done
                current_block->tick_info[m].steps_to_move = 0;
                motor[m]->stop_moving(); // let motor know it is no longer moving
            }
        }

        // see if any motors are still moving after this tick
        if(motor[m]->is_moving()) still_moving = true;
    }

    // do this after so we start at tick 0
    current_tick++; // count number of ticks

    // We may have set a pin on in this tick, now we reset the timer to set it off
    // Note there could be a race here if we run another tick before the unsteps have happened,
    // right now it takes about 3-4us but if the unstep were near 10uS or greater it would be an issue
    // also it takes at least 2us to get here so even when set to 1us pulse width it will still be about 3us
    if( unstep != 0) {
        start_unstep_ticker();
    }

    // see if any motors are still moving
    if(!still_moving) {
        //SET_STEPTICKER_DEBUG_PIN(0);

        // all moves finished
        current_tick = 0;

        // get next block
        // do it here so there is no delay in ticks
        Conveyor::getInstance()->block_finished();

        if(Conveyor::getInstance()->get_next_block(&current_block)) { // returns false if no new block is available
            running = start_next_block(); // returns true if there is at least one motor with steps to issue

        } else {
            current_block = nullptr;
            running = false;
        }

        // all moves finished
        // we delegate the slow stuff to the pendsv handler which will run as soon as this interrupt exits
        //NVIC_SetPendingIRQ(PendSV_IRQn); this doesn't work
        //SCB->ICSR = 0x10000000; // SCB_ICSR_PENDSVSET_Msk;
    }
}

// only called from the step tick ISR (single consumer)
__ramfunc__ bool StepTicker::start_next_block()
{
    if(current_block == nullptr) return false;

    bool ok = false;
    // need to prepare each active motor
    for (uint8_t m = 0; m < num_motors; m++) {
        if(current_block->tick_info[m].steps_to_move == 0) continue;

        ok = true; // mark at least one motor is moving
        // set direction bit here
        // NOTE this would be at least 10us before first step pulse.
        // TODO does this need to be done sooner, if so how without delaying next tick
        motor[m]->set_direction(current_block->direction_bits[m]);
        motor[m]->start_moving(); // also let motor know it is moving now
    }

    current_tick = 0;

    if(ok) {
        //SET_STEPTICKER_DEBUG_PIN(1);
        return true;

    } else {
        // this is an edge condition that should never happen, but we need to discard this block if it ever does
        // basically it is a block that has zero steps for all motors
        Conveyor::getInstance()->block_finished();
    }

    return false;
}


// returns index of the stepper motor in the array and bitset
int StepTicker::register_actuator(StepperMotor* m)
{
    motor[num_motors++] = m;
    return num_motors - 1;
}
