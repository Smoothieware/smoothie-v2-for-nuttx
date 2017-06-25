#include "StepperMotor.h"
#include "StepTicker.h"

//#include "Kernel.h"

#include <math.h>

StepperMotor::StepperMotor(Pin &step, Pin &dir, Pin &en) : step_pin(step), dir_pin(dir), en_pin(en)
{
    if(en.connected()) {
        //TODO how to do enable
        //set_high_on_debug(en.port_number, en.pin);
    }

    steps_per_mm         = 1.0F;
    max_rate             = 50.0F;

    last_milestone_steps = 0;
    last_milestone_mm    = 0.0F;
    current_position_steps= 0;
    moving= false;
    acceleration= NAN;
    selected= true;
    extruder= false;

    enable(false);
    unstep(); // initialize step pin
    set_direction(false); // initialize dir pin
}

StepperMotor::~StepperMotor()
{
}

// void StepperMotor::on_halt(void *argument)
// {
//     if(argument == nullptr) {
//         enable(false);
//         moving= false;
//     }
// }

// TODO how to do enable
// void StepperMotor::on_enable(void *argument)
// {
//     // argument is a uin32_t where bit0 is on or off, and bit 1:X, 2:Y, 3:Z, 4:A, 5:B, 6:C etc
//     // for now if bit0 is 1 we turn all on, if 0 we turn all off otherwise we turn selected axis off
//     uint32_t bm= (uint32_t)argument;
//     if(bm == 0x01) {
//         enable(true);

//     }else if(bm == 0 || ((bm&0x01) == 0 && ((bm&(0x02<<motor_id)) != 0)) ) {
//         enable(false);
//     }
// }

void StepperMotor::change_steps_per_mm(float new_steps)
{
    steps_per_mm = new_steps;
    last_milestone_steps = roundf(last_milestone_mm * steps_per_mm);
    current_position_steps = last_milestone_steps;
}

void StepperMotor::change_last_milestone(float new_milestone)
{
    last_milestone_mm = new_milestone;
    last_milestone_steps = roundf(last_milestone_mm * steps_per_mm);
    current_position_steps = last_milestone_steps;
}

void StepperMotor::set_last_milestones(float mm, int32_t steps)
{
    last_milestone_mm= mm;
    last_milestone_steps= steps;
    current_position_steps= last_milestone_steps;
}

void StepperMotor::update_last_milestones(float mm, int32_t steps)
{
    last_milestone_steps += steps;
    last_milestone_mm = mm;
}

int32_t StepperMotor::steps_to_target(float target)
{
    int32_t target_steps = roundf(target * steps_per_mm);
    return target_steps - last_milestone_steps;
}

// Does a manual step pulse, used for direct encoder control of a stepper
// NOTE this is experimental and may change and/or be removed in the future, it is an unsupported feature.
// use at your own risk
void StepperMotor::manual_step(bool dir)
{
    if(!is_enabled()) enable(true);

    // set direction if needed
    if(this->direction != dir) {
        this->direction= dir;
        this->dir_pin.set(dir);
        //wait_us(1);
    }

    // pulse step pin
    this->step_pin.set(1);
    //wait_us(3);
    this->step_pin.set(0);


    // keep track of actuators actual position in steps
    this->current_position_steps += (dir ? -1 : 1);
}
